#pragma once 

#include <functional>
#include <memory>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using closeCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer* buffer, Timestamp)>;

using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr& , size_t)>;