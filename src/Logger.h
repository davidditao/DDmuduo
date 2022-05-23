#pragma once

#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(LogmsgFormat, ...)          \
	do                                       \
	{                                        \
		Logger &logger = Logger::instance(); \
		logger.setLogLevel(INFO);            \
		char buf[1024] = {0};                \
		snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);  \
		logger.log(buf);                     \
	} while (0)

#define LOG_ERROR(LogmsgFormat, ...)         \
	do                                       \
	{                                        \
		Logger &logger = Logger::instance(); \
		logger.setLogLevel(ERROR);           \
		char buf[1024] = {0};                \
		snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);  \
		logger.log(buf);                     \
	} while (0)

#define LOG_FATAL(LogmsgFormat, ...)         \
	do                                       \
	{                                        \
		Logger &logger = Logger::instance(); \
		logger.setLogLevel(FATAL);           \
		char buf[1024] = {0};                \
		snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);  \
		logger.log(buf);                     \
		exit(-1);							 \
	} while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)         \
	do                                       \
	{                                        \
		Logger &logger = Logger::instance(); \
		logger.setLogLevel(DEBUG);           \
		char buf[1024] = {0};                \
		snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);  \
		logger.log(buf);                     \
	} while (0) 
#else
#define LOG_DEBUG(LogmsgFormat, ...) ;
#endif

// 定义日志的级别：
enum LogLevel
{
	INFO,  // 重要的流程信息
	ERROR, // 不影响程序继续运行的错误信息
	FATAL, // 影响程序继续运行的致命信息
	DEBUG, // 调试信息，可选可不选
};

// 线程安全的懒汉式单例模式
class Logger : noncopyable
{
public:
	// 获取日志唯一的实例对象
	static Logger &instance();

	// 设置日志级别
	void setLogLevel(int level);

	// 写日志
	void log(std::string msg);

private:
	int logLevel_;
	Logger() {}
};
