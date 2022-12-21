#include "Logger.h"
#include "Timestamp.h"

//单例
Logger& Logger::Instance(){
    static Logger logger;
    return logger;
}

//设置级别
void Logger::setLogLevel(LogLevel level){
    logLevel_ = level;
}

//写日志 格式：[日志级别] 时间  日志信息
void Logger::log(std::string msg){
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO] ";
        break;
    case ERROR:
        std::cout << "[ERROR] ";
        break;
    case DEBUG:
        std::cout << "[ERROR] ";
        break;
    case FATAL:
        std::cout << "[FATAL] ";
        break;
    default:
        break;
    }
    //打印时间和msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}