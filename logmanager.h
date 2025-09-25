#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H
#pragma once


#include <string>
#include "logstream.h"


struct LogConfig {
    std::string logger_name = "log";    // 日志名称
    std::string filepath = "logs";      // 日志文件路径
    std::string filename = "log.txt";   // 日志文件名称
    int level = 1;                     // 日志等级 trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5, off = 6
    int max_size = 1024 * 1024 * 50;   // 单个日志文本大小
    int days_to_keep = 10;             // 保留日志天数
    bool auto_cleanup = true;          // 是否自动清理日志
    bool console = true;               // 是否输出到控制台
};

class LogManagerPrivate;
class LogManager {
public:
   // 获取单例实例
   static LogManager& instance();

   /**
    * @brief 初始化日志系统
    * @param q_size         队列大小
    * @param thread_count   工作线程数
    * @return
    */
 void init(int q_size = 8192, int thread_count = 1);

 /**
    * @brief 添加日志配置
    * @param config         日志配置
    * @return
    */
 void addConfig(const LogConfig& config = LogConfig()) ;

   /*!
    * @brief 设置日志级别
    * @param level 日志等级 trace = 0, debug = 1 , info = 2 , warn = 3 , err = 4 , critical = 5 , off = 6
    */
   void setLevel(int level = 2) const;

   // 创建日志流
   LogStream trace(const std::string& logger_name="log") const;
   LogStream debug(const std::string& logger_name="log") const;
   LogStream info(const std::string& logger_name="log") const;
   LogStream warn(const std::string& logger_name="log") const;
   LogStream error(const std::string& logger_name="log") const;
   LogStream critical(const std::string& logger_name="log") const;

   /*!
    * @brief 清理日志, 在应用首次启动时,会自动执行清理函数,
    *        在应用运行时, 可手动调用此函数清理日志,
    * @param days_to_keep  保留天数, 默认保留10天
    */
   void cleanup(int days_to_keep = 10);


   /*!
    * @brief 启动日志清理线程
    *        应用运行时在每天0点执行一次清理日志
    *        也可直接设置auto_cleanup 参数直接关闭清理日志线程.
    *        调用此函数默认会开启清理日志线程,关闭应用时销毁清理日志该线程.
    * @param auto_cleanup  是否自动清理日志
    */
   void startTask(bool auto_cleanup = true);

private:
   LogManager();
   ~LogManager();
   // 禁止复制和赋值
   LogManager(const LogManager&) = delete;
   LogManager& operator=(const LogManager&) = delete;

 private:
     LogManagerPrivate* const d_ptr;
 };
/**
* @brief 初始化日志系统
* @param q_size         队列大小  默认 8192
* @param thread_count   工作线程数  默认 1
 * @return
*/
#define LogInit            LogManager::instance().init

/**
 *
 * @brief 初始化日志系统  可以多次初始化 创建不同的日志名称
* @param config         日志配置
      * @param logger_name  日志名称, 默认log
      * @param filepath     日志文件路径, 默认logs
      * @param filename     日志文件名称, 默认log.txt
      * @param level        日志等级 trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5, off = 6, 默认info
      * @param max_size     单个日志文本大小, 默认50M
      * @param days_to_keep 保留日志天数, 默认10天
      * @param auto_cleanup 是否自动清理日志, 默认true
 * @return
*/
#define LogAddConfig            LogManager::instance().addConfig

/*!
 * @brief 设置日志级别
 * @param level 日志等级 trace = 0, debug = 1 , info = 2 , warn = 3 , err = 4 , critical = 5 , off = 6
 */
#define LogSetLevel   LogManager::instance().setLevel

/*!
 * @brief 清理日志, 在应用首次启动时,会自动执行清理函数,
 *        在应用运行过程中, 可手动调用此函数清理日志,
 *        调用此函数默认会开启清理日志线程,关闭应用时销毁清理日志该线程,
 *        应用运行时在每天0点执行一次清理日志:
 * @param days_to_keep  保留天数, 默认保留10天
 */
#define LogCleanup         LogManager::instance().cleanup

// 创建日志流
#define LogTrace           LogManager::instance().trace
#define LogDebug           LogManager::instance().debug
#define LogInfo            LogManager::instance().info
#define LogWarn            LogManager::instance().warn
#define LogError           LogManager::instance().error
#define LogCritical        LogManager::instance().critical


#endif // LOG_MANAGER_H
