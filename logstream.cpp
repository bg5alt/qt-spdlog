#include "logstream.h"
#include "logmanager_p.h"

#include <spdlog/spdlog.h>

#include <utility>

LogStream::LogStream(const int level, const std::shared_ptr<spdlog::logger>& logger)
    : _level(level), _logger(logger) {
    if (_logger) {
        _enabled = _logger->should_log(LogManagerPrivate::toSpdlogLevel(_level));
    }
}

// 析构函数，在对象销毁时记录日志
LogStream::~LogStream() {
    if (!_enabled || !_logger) {
        return;
    }
    try {
        const std::string msg = _stream.str();
        if (msg.empty()) {
            return;
        }
        _logger->log(LogManagerPrivate::toSpdlogLevel(_level), msg);
    } catch (const std::exception& e) {
        // 捕获异常，避免在析构函数中崩溃
        (void)e;
    } catch (...) {
        // 捕获所有异常，避免在析构函数中崩溃
    }
}
