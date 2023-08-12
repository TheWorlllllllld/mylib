#include <mymuduo/TcpClient.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>
#include <iostream>

#include <thread>


std::string message = "Hello\n";
std::string message2 = "World\n";

void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(),
           conn->peerAddress().toIpPort().c_str());
    conn->send(message);
  }
  else
  {
    printf("onConnection(): connection [%s] is down\n",
           conn->name().c_str());
  }
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toString().c_str());

  printf("onMessage(): [%s]\n", buf->retrieveAllAsString().c_str());
}

int main()
{
  EventLoop loop;
  InetAddress serverAddr(8000,"127.0.0.1");
  TcpClient client(&loop, serverAddr, "TcpClient");

  // client2
  TcpClient client2(&loop, serverAddr, "TcpClient2");

  // 创建一个线程，每隔2秒发送一次消息
  std::thread t([&](){
    while(true){
      std::this_thread::sleep_for(std::chrono::seconds(2));
      client.connection()->send(message);
      client2.connection()->send(message2);
    }
  });

  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.enableRetry();
  client.connect();

  client2.setConnectionCallback(onConnection);
  client2.setMessageCallback(onMessage);
  client2.enableRetry();
  client2.connect();

  loop.loop();
}