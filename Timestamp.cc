#include "Timestamp.h"

#include <time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::now(){
    time_t now = time(NULL);
    return Timestamp(now);
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
