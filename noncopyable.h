#pragma once

/**
 * @brief The NonCopyable class
 * 当nocopyable类被继承时，派生类不能被拷贝，不能被拷贝构造，不能被赋值
 * 但却可以进行正常的构造和析构
 */

class noncopyable 
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};