#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    static const int kMicroSecondsPerSecond = 1000 * 1000;

public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // 重载运算符
public:
    inline friend bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline friend bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    inline friend bool operator>(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
    }

private:
    // 微秒
    int64_t microSecondsSinceEpoch_;
};

inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTimer(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}