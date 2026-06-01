#ifndef LOG_STREAM_H
#define LOG_STREAM_H
#pragma once

#include <memory>

#include "estream.h"

namespace spdlog {
class logger;
} // namespace spdlog

class LogStream {
public:
    LogStream(int level, const std::shared_ptr<spdlog::logger>& logger);

    // 析构函数，在对象销毁时记录日志
    ~LogStream();

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&&) noexcept = default;
    LogStream& operator=(LogStream&&) noexcept = default;

    // 通用模板
    template <typename T>
    LogStream& operator<<(const T& value) {
        if (_enabled) {
            _stream << value;
        }
        return *this;
    }

private:
    int _level = 2;
    bool _enabled = false;
    std::shared_ptr<spdlog::logger> _logger;
    EStream _stream;
};

#endif // LOG_STREAM_H
