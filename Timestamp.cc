#include "Timestamp.h"

#include <time.h>
#include <sys/time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch) {}


// 必须达到微妙级别才可以，直接用time函数是秒级别的不可以
Timestamp Timestamp::now(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t  seconds = tv.tv_sec;   // 秒
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
std::string Timestamp::toString() const{
    char buf[128] = {0};
    tm *tm_ = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d-%02d-%02d %02d:%02d:%02d", 
                        tm_->tm_year + 1900, 
                        tm_->tm_mon + 1, 
                        tm_->tm_mday, 
                        tm_->tm_hour, 
                        tm_->tm_min, 
                        tm_->tm_sec);
    return buf;
}
