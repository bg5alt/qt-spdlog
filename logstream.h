#ifndef LOG_STREAM_H
#define LOG_STREAM_H
#pragma once

#include <memory>
#include "estream.h"


class LogStreamImpl; // 前向声明实现类
class LogStream {
public:
    explicit LogStream(int level, const std::shared_ptr<void>& logger);

    // 析构函数，在对象销毁时记录日志
    ~LogStream();
    // // 通用模板
    template <typename T>
    LogStream& operator<<(const T& value) {
        _stream << value;
        return *this;
    }

private:
    std::unique_ptr<LogStreamImpl> impl; // Pimpl 隐藏实现
    EStream _stream;
};




#endif // LOG_STREAM_H
