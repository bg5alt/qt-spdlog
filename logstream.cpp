#include "logstream.h"
#include "logmanager_p.h"

#include <spdlog/spdlog.h>

#include <utility>


class LogStreamImpl {
public:
    LogStreamImpl(spdlog::level::level_enum level, std::shared_ptr<spdlog::logger> logger)
        : level(level), logger(std::move(logger)) {}

    spdlog::level::level_enum level;
    std::shared_ptr<spdlog::logger> logger;
};


LogStream::LogStream(const int level, const std::shared_ptr<void>& logger) {
    if (logger) {
        impl = std::make_unique<LogStreamImpl>(LogManagerPrivate::toSpdlogLevel(level), std::static_pointer_cast<spdlog::logger>(logger));
    }
}

// 析构函数，在对象销毁时记录日志
LogStream::~LogStream() {
    try {
        if (impl && impl->logger) {
            // 检查logger是否有效
            std::string msg = _stream.str();
            if (!msg.empty()) {
                // 根据日志级别记录日志
                switch (impl->level) {
                case spdlog::level::trace:
                    impl->logger->trace(msg);
                    break;
                case spdlog::level::debug:
                    impl->logger->debug(msg);
                    break;
                case spdlog::level::info:
                    impl->logger->info(msg);
                    break;
                case spdlog::level::warn:
                    impl->logger->warn(msg);
                    break;
                case spdlog::level::err:
                    impl->logger->error(msg);
                    break;
                case spdlog::level::critical:
                    impl->logger->critical(msg);
                    break;
                default:
                    impl->logger->info(msg);
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        // 捕获异常，避免在析构函数中崩溃
    } catch (...) {
        // 捕获所有异常，避免在析构函数中崩溃
    }
}


