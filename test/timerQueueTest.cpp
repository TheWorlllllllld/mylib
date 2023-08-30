#include <mymuduo/TcpClient.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>
#include <iostream>

#include <thread>

using namespace std;

int main()
{
    EventLoop loop;
    TimerQueue *Queue = loop.getTimerQueue();
    auto a = [&]()
    {
        std::cout << "send message" << std::endl;
    };

    // loop.runAfter(4, a);
    // loop.runEvery(1, a);
    // loop.runAfter(3, a);
    loop.runAt(Timestamp(4),a);
    LOG_INFO("time: %s",Timestamp(4).toString().c_str());



    loop.loop();
}