#include "logmanager.h"
#include "logmanager_p.h"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

#include "estream.h"

#ifdef _WIN32
#include <windows.h>  // Windows 控制台编码控制
#endif

// 定义日志级别映射
std::map<int, spdlog::level::level_enum> log_level_map = {
    {0, spdlog::level::trace},
    {1, spdlog::level::debug},
    {2, spdlog::level::info},
    {3, spdlog::level::warn},
    {4, spdlog::level::err},
    {5, spdlog::level::critical},
    {6, spdlog::level::off}
};

// 实现转换函数
spdlog::level::level_enum LogManagerPrivate::toSpdlogLevel(int level) {
    const auto& iter = log_level_map.find(level);
    if (iter != log_level_map.end() ) {
        return iter->second;
    }
    return spdlog::level::info;
}

std::shared_ptr<spdlog::logger> LogManagerPrivate::getLogger(const std::string& name) {
    /// 按照名称取一个
    auto iter = _loggers.find(name);
    if (iter != _loggers.end()) {
        return iter->second;
    }
    /// 没找到 就取第一个
    auto it = _loggers.begin();
    if (it != _loggers.end()) {
        return it->second;
    }
    /// 都没有 就返回默认logger
    return spdlog::default_logger();
}

// 获取单例实例
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}
LogManager::LogManager() : d_ptr(new LogManagerPrivate) {

}
// 完全移除析构函数，让系统自动处理资源的释放
// LogManager::~LogManager() {
//     try {
//         // 首先通知线程退出
//         d_ptr->_cleanup_thread_running = false;
//         // 等待线程结束
//         if (d_ptr->_cleanup_thread.joinable()) {
//             d_ptr->_cleanup_thread.join(); // 等待线程结束
//         }
//         // 等待一段时间，确保所有LogStream对象都已经被销毁
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         // 确保所有日志在程序结束前被刷新
//         // 避免调用clear()，直接让_loggers的析构函数处理
//         // d_ptr->_loggers.clear();
//         // 释放资源
//         // d_ptr是const指针，不能修改其值，只能让系统自动释放
//     } catch (const std::exception& e) {
//         // 捕获异常，避免在析构函数中崩溃
//     } catch (...) {
//         // 捕获所有异常，避免在析构函数中崩溃
//     }
// }

// 初始化日志系统
void LogManager::init(const int q_size, const int thread_count ) {
    if (!d_ptr->_init) {
        d_ptr->_init = true;
#ifdef _WIN32
        // 设置输入输出编码为 UTF-8
        SetConsoleCP(65001);   // 控制台输入编码
        SetConsoleOutputCP(65001);  // 控制台输出编码
#endif
        // 第一个参数是队列大小，第二个参数是工作线程数
        // 注释掉线程池初始化，使用同步模式
        // spdlog::init_thread_pool(q_size, thread_count);

        // 初始化spdlog
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [pid:%P] [thread:%t] [%n] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::trace);
    }
}

void LogManager::addConfig(const LogConfig& config)
{
    try {
        // 1. 确保日志目录存在
        std::filesystem::create_directories(config.filepath);

        // 2. 创建sinks
        std::vector<spdlog::sink_ptr> sinks;

        // 3. 文件sink
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.filepath + "/" + config.filename, config.max_size, 100000);
        sinks.push_back(file_sink);

        // 4. 控制台sink（始终添加，确保在控制台中看到日志输出）
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace); // 设置控制台sink的日志级别为trace
        sinks.push_back(console_sink);

        // 5. 创建logger
        auto logger = std::make_shared<spdlog::logger>(config.logger_name, sinks.begin(), sinks.end());

        // 6. 设置日志格式和级别
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [pid:%P] [thread:%t] [%n] [%^%l%$] %v");
        logger->set_level(d_ptr->toSpdlogLevel(config.level));
        logger->flush_on(d_ptr->toSpdlogLevel(config.level));

        // 7. 注册到spdlog全局注册表
        spdlog::register_logger(logger);

        // 8. 存储到本地日志器映射
        d_ptr->_loggers[config.logger_name] = logger;

        d_ptr->_logger_dirs.push_back(config.filepath);

        // 9. 设置第一个logger为默认logger（可选）
        if (d_ptr->_loggers.size() == 1) {
            d_ptr->_cleanup_days_to_keep = config.days_to_keep;
            d_ptr->_cleanup_auto = config.auto_cleanup;
            // 设置为默认logger
            spdlog::set_default_logger(logger);
            // 设置全局日志级别为trace
            spdlog::set_level(spdlog::level::trace);
            // 启动日志清理线程
            this->startTask(d_ptr->_cleanup_auto);
        }
    } catch (const spdlog::spdlog_ex& ex) {
        // 异常处理（如文件创建失败）
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    } catch (const std::exception& e) {
        // 捕获其他异常
        std::cerr << "Exception in addConfig: " << e.what() << std::endl;
    }
}


// 设置日志级别
void LogManager::setLevel(int level) const {

    for (auto& pair : d_ptr->_loggers) {
        auto logger = pair.second;
        logger->set_level(d_ptr->toSpdlogLevel(level));
    }
    // 移除spdlog::set_level调用，避免在析构函数中崩溃
    // spdlog::set_level(d_ptr->toSpdlogLevel(level));
}

// 创建日志流
LogStream LogManager::trace(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 0, std::static_pointer_cast<void>(logger));
}

LogStream LogManager::debug(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 1, std::static_pointer_cast<void>(logger));
}

LogStream LogManager::info(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 2, std::static_pointer_cast<void>(logger));
}
LogStream LogManager::warn(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 3, std::static_pointer_cast<void>(logger));
}

LogStream LogManager::error(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 4, std::static_pointer_cast<void>(logger));
}

LogStream LogManager::critical(const std::string& logger_name) const {
    auto logger = d_ptr->getLogger(logger_name);
    return LogStream( 5, std::static_pointer_cast<void>(logger));
}



void LogManager::cleanup(int days_to_keep) {
    if (!d_ptr || d_ptr->_logger_dirs.empty()) return;

    const auto now = std::chrono::system_clock::now();
    const auto cutoff_time = now - std::chrono::hours(24 * days_to_keep);
    // 测试 删除 1 分钟 之前的文件
    // const auto cutoff_time = now - std::chrono::minutes(1);

    for (auto& dir : d_ptr->_logger_dirs) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (std::filesystem::is_regular_file(entry)) {
                // 检查文件是否被占用
                std::ifstream file(entry.path());
                if (!file.is_open()) {
                    // 文件被占用，跳过处理
                    continue;
                }
                file.close();

                auto last_write_time = std::filesystem::last_write_time(entry);
                // 转换为系统时间点进行比较
                auto sys_last_write_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    last_write_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
                );

                if (sys_last_write_time < cutoff_time) {
                    try
                    {
                        std::filesystem::remove(entry.path());
                    }
                    catch(const std::exception& e)
                    {
                    }

                }
            }
        }
    }

    if( d_ptr->_cleanup_days_to_keep != days_to_keep) {
        d_ptr->_cleanup_days_to_keep = days_to_keep;
        startTask(d_ptr->_cleanup_auto);
    }

}


// 启动日志清理线程
void LogManager::startTask(bool auto_cleanup) {
    if (!auto_cleanup) {
        d_ptr->_cleanup_auto = auto_cleanup;
        d_ptr->_cleanup_thread_running = false; // 通知线程退出
        d_ptr->_cleanup_time_started = false;
        if (d_ptr->_cleanup_thread.joinable()) {
            d_ptr->_cleanup_thread.join(); // 等待线程结束
        }
    }
    /// 启用  自动清理线程
    if (d_ptr->_cleanup_auto) {
        if (!d_ptr->_cleanup_time_started) {
            d_ptr->_cleanup_time_started = true;
            // 完全移除清理线程，避免互斥量锁定失败
        }
    }
}