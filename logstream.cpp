#include "logstream.h"
#include "logmanager_p.h"

#include <spdlog/spdlog.h>


class LogStreamImpl {
public:
    LogStreamImpl(spdlog::level::level_enum level, std::shared_ptr<spdlog::logger> logger)
        : level(level), logger(logger) {}

    spdlog::level::level_enum level;
    std::shared_ptr<spdlog::logger> logger;
};


LogStream::LogStream(int level, const std::shared_ptr<void>& logger) {
    impl = std::make_unique<LogStreamImpl>(LogManagerPrivate::toSpdlogLevel(level), std::static_pointer_cast<spdlog::logger>(logger));
}

// 析构函数，在对象销毁时记录日志
LogStream::~LogStream() {
    if (impl && impl->logger && impl->logger->should_log(impl->level)) {
        impl->logger->log(impl->level, static_cast<std::string>(_stream));
        impl->logger->flush();
    }
}


