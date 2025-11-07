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
    /// 都没有 就返回空
    return nullptr;
}

// 获取单例实例
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}
LogManager::LogManager() : d_ptr(new LogManagerPrivate) {

}
LogManager::~LogManager() {
    // 确保所有日志在程序结束前被刷新
    // spdlog::shutdown();
    d_ptr->_cleanup_thread_running = false; // 通知线程退出
    if (d_ptr->_cleanup_thread.joinable()) {
        d_ptr->_cleanup_thread.join(); // 等待线程结束
    }
    delete d_ptr;
}

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
        spdlog::init_thread_pool(q_size, thread_count);
    }
}

void LogManager::addConfig(const LogConfig& config)
{
    if (!d_ptr->_loggers.contains(config.logger_name)) {
        try {
            // 1. 文件的 sink
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.filepath + "/" + config.filename, config.max_size, 100000);

            // 2. 将 sink 组合到 vector 中
            std::vector<spdlog::sink_ptr> sinks;
            if (config.console) {
                /// 创建控制台 sink
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sinks.push_back(console_sink);
            }
            sinks.push_back(file_sink);


            // 3. 创建 async_logger 并添加 sinks
            // auto logger = std::make_shared<spdlog::logger>(config.logger_name, sinks.begin(), sinks.end());
            auto logger = std::make_shared<spdlog::async_logger>(config.logger_name, sinks.begin(), sinks.end(),
                                                                 spdlog::thread_pool(),
                                                                 spdlog::async_overflow_policy::block);

            // 4. 设置日志格式和级别
            // logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
            logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [pid:%P] [thread:%t] [%n] [%^%l%$] %v");
            logger->set_level(d_ptr->toSpdlogLevel(config.level));
            logger->flush_on(d_ptr->toSpdlogLevel(config.level));
            d_ptr->_loggers[config.logger_name] = logger;

            d_ptr->_logger_dirs.push_back(config.filepath);
            // 5. 注册为默认 logger（可选）
            // _logger = spdlog::basic_logger_mt(logger_name, filename);
            // 设置第一个logger为默认logger（可选）
            if (d_ptr->_loggers.size() == 1) {
                spdlog::set_default_logger(logger);
                spdlog::set_level(d_ptr->toSpdlogLevel(config.level)); // 设置默认日志级别
                spdlog::flush_on(d_ptr->toSpdlogLevel(config.level));  // 设置日志刷新级别
                d_ptr->_cleanup_days_to_keep = config.days_to_keep;
                d_ptr->_cleanup_auto = config.auto_cleanup;
                // 启动日志清理线程
                this->startTask(d_ptr->_cleanup_auto);
            }
        } catch (const spdlog::spdlog_ex& ex) {
            // 异常处理（如文件创建失败）
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }

}


// 设置日志级别
void LogManager::setLevel(int level) const {

    for (auto& pair : d_ptr->_loggers) {
        auto logger = pair.second;
        logger->set_level(d_ptr->toSpdlogLevel(level));
    }
    spdlog::set_level(d_ptr->toSpdlogLevel(level));
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
                    info() << "File is locked, skip: " << entry.path().string();
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
                        info() << "Deleted old log file: " << entry.path().string();
                    }
                    catch(const std::exception& e)
                    {
                        info() << "Failed to delete old log file: " << entry.path().string() << " error: " << e.what();
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
        {
            std::lock_guard<std::mutex> lock(d_ptr->_cleanup_mutex);
            d_ptr->_cleanup_thread_running = false; // 通知线程退出
        }
        d_ptr->_cleanup_time_started = false;
        if (d_ptr->_cleanup_thread.joinable()) {
            d_ptr->_cleanup_thread.join(); // 等待线程结束
        }
    }
    /// 启用  自动清理线程
    if (d_ptr->_cleanup_auto) {
        std::lock_guard<std::mutex> lock(d_ptr->_cleanup_mutex);

        if (!d_ptr->_cleanup_time_started) {
            d_ptr->_cleanup_time_started = true;
            d_ptr->_cleanup_thread_running = true;
            d_ptr->_cleanup_thread = std::thread([this]() {
                while (d_ptr->_cleanup_thread_running) {
                    // 获取当前系统时间
                    const auto _now = std::chrono::system_clock::now();
                    
                    // 跨平台方式计算次日0点
                    auto now_time_t = std::chrono::system_clock::to_time_t(_now);
                    #ifdef _WIN32
                        struct tm tm;
                        localtime_s(&tm, &now_time_t);
                        tm.tm_hour = 24; tm.tm_min = 0; tm.tm_sec = 0;
                        auto next_midnight = std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
                    #else
                        struct tm* tm = localtime(&now_time_t);
                        tm->tm_hour = 24; tm->tm_min = 0; tm->tm_sec = 0;
                        auto next_midnight = std::chrono::system_clock::from_time_t(mktime(tm));
                    #endif
                    
                    // 计算等待时长
                    auto duration = next_midnight - _now;
                    // 等待到目标时间
                    using DurationType = decltype(duration);
                    if (duration > DurationType::zero()) {
                        std::this_thread::sleep_for(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
                        );
                    }

                    // // 测试 每30秒执行一次
                    // std::this_thread::sleep_for(std::chrono::seconds(30));

                    info() << "start_task: delete " ;
                    // 执行清理（使用首次调用时的参数）
                    if (d_ptr->_cleanup_thread_running) {
                        this->cleanup(d_ptr->_cleanup_days_to_keep);
                    }
                }
            });
        }
    }
}