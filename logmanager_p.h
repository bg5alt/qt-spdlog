#ifndef LOG_MANAGER_P_H
#define LOG_MANAGER_P_H
#pragma once


#include <string>

#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

class LogManagerPrivate {
public:
    // 日志器
    std::map<std::string, std::shared_ptr<spdlog::logger>> _loggers;
    std::vector<std::string>                               _logger_dirs;
    // 添加静态转换函数
    //Trace = 0, Debug = 1 , Info = 2 , Warn = 3 , Err = 4 , Critical = 5 , Off = 6
    static spdlog::level::level_enum toSpdlogLevel(int level = 2);

    // 定时清理日志任务线程
    std::thread         _cleanup_thread;                    //  定时清理日志任务线程
    std::mutex          _cleanup_mutex;                     //  定时清理日志任务线程锁
    std::atomic<bool>   _cleanup_thread_running = false;    //  线程运行标志
    std::atomic<bool>   _cleanup_time_started = false;      //  定时任务启动标志
    int                 _cleanup_days_to_keep = 10;         //  保留天数
    bool                _cleanup_auto = false;              //  是否自动清理日志
    bool                _init = false;                      //  是否初始化

    std::shared_ptr<spdlog::logger> getLogger(const std::string& name);
};



#endif // LOG_MANAGER_P_H
