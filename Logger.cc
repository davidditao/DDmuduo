#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 线程安全的懒汉式单例模式

// 获取日志唯一的实例对象
Logger &Logger::instance()
{
	static Logger logger;
	return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
	logLevel_ = level;
}

// 写日志
void Logger::log(std::string msg)
{
	// 打印日志级别
	switch (logLevel_)
	{
	case INFO:
		std::cout << "[INFO]";
		break;
	case ERROR:
		std::cout << "[ERROR]";
		break;
	case FATAL:
		std::cout << "[FATAL]";
		break;
	case DEBUG:
		std::cout << "[DEBUG]";
		break;
	default:
		break;
	}

	// 打印时间和信息
	std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}
