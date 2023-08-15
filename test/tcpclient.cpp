#include <mymuduo/TcpClient.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>
#include <iostream>

#include <thread>

using namespace std;

std::string message = "Hello\n";
std::string message2 = "World\n";

void onConnection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(),
           conn->peerAddress().toIpPort().c_str());
    // conn->send(message);
  }
  else
  {
    printf("onConnection(): connection [%s] is down\n",
           conn->name().c_str());
  }
}

void onMessage(const TcpConnectionPtr &conn,
               Buffer *buf,
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
  InetAddress serverAddr(8000, "127.0.0.1");
  TcpClient client(&loop, serverAddr, "TcpClient");

  auto a = [&]()
  {
    client.connection()->send(message);
    // std::cout<<"send message"<<std::endl;
  };

  thread t([&]()
  {
    while(1){
      sleep(2);
      a();
    }
  });

  loop.runEvery(1, a);

  TimerQueue* Queue = loop.getTimerQueue();
  cout<<Queue->timerlen()<<endl;
  cout<<Queue->activeTimerlen()<<endl;
  cout<<Queue->cancelingTimerlen()<<endl;

  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.enableRetry();
  client.connect();
  loop.loop();
}