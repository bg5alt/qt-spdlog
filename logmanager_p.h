#ifndef LOG_MANAGER_P_H
#define LOG_MANAGER_P_H
#pragma once


#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

class LogManagerPrivate {
public:
    // 添加静态转换函数
    //Trace = 0, Debug = 1 , Info = 2 , Warn = 3 , Err = 4 , Critical = 5 , Off = 6
    static spdlog::level::level_enum toSpdlogLevel(int level);

    std::shared_ptr<spdlog::logger> getLogger(const std::string& name) const;

    // 日志器
    mutable std::recursive_mutex _mutex;
    std::map<std::string, std::shared_ptr<spdlog::logger>> _loggers;
    std::vector<std::string>                               _logger_dirs;
    std::set<std::string>                                  _active_log_files; // 当前进程正在写入的日志文件

    int  _cleanup_days_to_keep = 10;         //  保留天数
    bool _cleanup_auto = false;              //  是否自动清理日志
    bool _append_pid = false;                //  是否使用进程ID隔离日志文件
    bool _init = false;                      //  是否初始化
    bool _shutdown = false;                  //  是否已关闭
    int  _process_id = 0;                    //  当前进程ID
};

#endif // LOG_MANAGER_P_H
