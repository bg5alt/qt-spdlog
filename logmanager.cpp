#include "logmanager.h"
#include "logmanager_p.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <windows.h>  // Windows 控制台编码控制
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

constexpr const char* kLogPattern = "[%Y-%m-%d %H:%M:%S.%e] [pid:%P] [thread:%t] [%n] [%^%l%$] %v";

int currentProcessId() {
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

std::string buildLogFilename(const LogConfig& config) {
    if (!config.append_pid) {
        return config.filename;
    }

    const auto path = std::filesystem::path(config.filename);
    const auto pid = currentProcessId();
    return path.stem().string() + "." + std::to_string(pid) + path.extension().string();
}

bool belongsToCurrentProcess(const std::string& filename, int pid) {
    const auto marker = "." + std::to_string(pid) + ".";
    return filename.find(marker) != std::string::npos;
}

} // namespace

// 实现转换函数
spdlog::level::level_enum LogManagerPrivate::toSpdlogLevel(const int level) {
    switch (level) {
    case 0:
        return spdlog::level::trace;
    case 1:
        return spdlog::level::debug;
    case 2:
        return spdlog::level::info;
    case 3:
        return spdlog::level::warn;
    case 4:
        return spdlog::level::err;
    case 5:
        return spdlog::level::critical;
    case 6:
        return spdlog::level::off;
    default:
        return spdlog::level::info;
    }
}

std::shared_ptr<spdlog::logger> LogManagerPrivate::getLogger(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_shutdown) {
        return nullptr;
    }

    /// 按照名称取一个
    if (const auto iter = _loggers.find(name); iter != _loggers.end()) {
        return iter->second;
    }
    /// 没找到 就取第一个
    if (!_loggers.empty()) {
        std::cerr << "[LogManager] logger '" << name << "' not found, fallback to '"
                  << _loggers.begin()->first << "'" << std::endl;
        return _loggers.begin()->second;
    }
    /// 都没有 就返回默认logger
    std::cerr << "[LogManager] no logger registered, using spdlog default logger" << std::endl;
    return spdlog::default_logger();
}

// 获取单例实例
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : d_ptr(new LogManagerPrivate) {
    d_ptr->_process_id = currentProcessId();
}

LogManager::~LogManager() {
    shutdown();
    delete d_ptr;
}

// 初始化日志系统
void LogManager::init(const int q_size, const int thread_count) {
    (void)q_size;
    (void)thread_count;

    std::lock_guard<std::recursive_mutex> lock(d_ptr->_mutex);
    if (d_ptr->_init || d_ptr->_shutdown) {
        return;
    }
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
    spdlog::set_pattern(kLogPattern);
    spdlog::set_level(spdlog::level::trace);
}

void LogManager::addConfig(const LogConfig& config) {
    if (d_ptr->_shutdown) {
        return;
    }

    try {
        std::lock_guard<std::recursive_mutex> lock(d_ptr->_mutex);
        if (d_ptr->_loggers.count(config.logger_name) > 0) {
            return;
        }

        // 1. 确保日志目录存在
        std::filesystem::create_directories(config.filepath);

        // 2. 创建sinks
        std::vector<spdlog::sink_ptr> sinks;

        // 3. 文件sink
        const auto filename = buildLogFilename(config);
        const auto log_path = (std::filesystem::path(config.filepath) / filename).string();
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_path, config.max_size, 100000));
        d_ptr->_active_log_files.insert(log_path);
        if (config.append_pid) {
            d_ptr->_append_pid = true;
        }

        // 4. 控制台sink
        if (config.console) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace); // 设置控制台sink的日志级别为trace
            sinks.push_back(console_sink);
        }

        // 5. 创建logger
        auto logger = std::make_shared<spdlog::logger>(config.logger_name, sinks.begin(), sinks.end());

        // 6. 设置日志格式和级别
        const auto spdlog_level = d_ptr->toSpdlogLevel(config.level);
        logger->set_pattern(kLogPattern);
        logger->set_level(spdlog_level);
        // warn 及以上级别才刷盘，避免 trace/debug 频繁 IO
        logger->flush_on(spdlog::level::warn);

        // 7. 注册到spdlog全局注册表
        spdlog::register_logger(logger);

        // 8. 存储到本地日志器映射
        d_ptr->_loggers[config.logger_name] = logger;
        d_ptr->_logger_dirs.push_back(config.filepath);

        // 9. 设置第一个logger为默认logger（可选）
        const bool first_logger = d_ptr->_loggers.size() == 1;
        if (first_logger) {
            d_ptr->_cleanup_days_to_keep = config.days_to_keep;
            d_ptr->_cleanup_auto = config.auto_cleanup;
            // 设置为默认logger
            spdlog::set_default_logger(logger);
            // 设置全局日志级别为trace
            spdlog::set_level(spdlog::level::trace);
        }

        if (first_logger && d_ptr->_cleanup_auto) {
            // 首次启动时自动清理过期日志
            cleanup(config.days_to_keep);
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
void LogManager::setLevel(const int level) const {
    if (d_ptr->_shutdown) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(d_ptr->_mutex);
    for (const auto& [_, logger] : d_ptr->_loggers) {
        logger->set_level(d_ptr->toSpdlogLevel(level));
    }
    // 移除spdlog::set_level调用，避免在析构函数中崩溃
    // spdlog::set_level(d_ptr->toSpdlogLevel(level));
}

LogStream LogManager::makeStream(const int level, const std::string& logger_name) const {
    return LogStream(level, d_ptr->getLogger(logger_name));
}

// 创建日志流
LogStream LogManager::trace(const std::string& logger_name) const {
    return makeStream(0, logger_name);
}

LogStream LogManager::debug(const std::string& logger_name) const {
    return makeStream(1, logger_name);
}

LogStream LogManager::info(const std::string& logger_name) const {
    return makeStream(2, logger_name);
}

LogStream LogManager::warn(const std::string& logger_name) const {
    return makeStream(3, logger_name);
}

LogStream LogManager::error(const std::string& logger_name) const {
    return makeStream(4, logger_name);
}

LogStream LogManager::critical(const std::string& logger_name) const {
    return makeStream(5, logger_name);
}

void LogManager::cleanup(const int days_to_keep) {
    if (!d_ptr || d_ptr->_shutdown) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(d_ptr->_mutex);
    if (d_ptr->_logger_dirs.empty()) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto cutoff_time = now - std::chrono::hours(24 * days_to_keep);
    // 测试 删除 1 分钟 之前的文件
    // const auto cutoff_time = now - std::chrono::minutes(1);

    for (const auto& dir : d_ptr->_logger_dirs) {
        if (!std::filesystem::exists(dir)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const auto file_path = entry.path().string();
            // 跳过当前进程正在写入的日志文件
            if (d_ptr->_active_log_files.count(file_path) > 0) {
                continue;
            }

            // 多进程模式下只清理本进程的日志文件
            if (d_ptr->_append_pid
                && !belongsToCurrentProcess(entry.path().filename().string(), d_ptr->_process_id)) {
                continue;
            }

            const auto last_write_time = entry.last_write_time();
            // 转换为系统时间点进行比较
            const auto sys_last_write_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                last_write_time - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now());

            if (sys_last_write_time >= cutoff_time) {
                continue;
            }

            try {
                std::filesystem::remove(entry.path());
            } catch (const std::exception& e) {
                // 文件被占用，跳过处理
                (void)e;
            }
        }
    }

    d_ptr->_cleanup_days_to_keep = days_to_keep;
}

void LogManager::shutdown() {
    if (!d_ptr || d_ptr->_shutdown) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(d_ptr->_mutex);
    if (d_ptr->_shutdown) {
        return;
    }

    for (const auto& [_, logger] : d_ptr->_loggers) {
        if (logger) {
            logger->flush();
        }
    }

    d_ptr->_loggers.clear();
    d_ptr->_active_log_files.clear();
    d_ptr->_logger_dirs.clear();
    spdlog::shutdown();
    d_ptr->_shutdown = true;
}
