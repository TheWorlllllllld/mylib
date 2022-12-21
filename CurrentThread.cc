#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if(t_cachedTid == 0)
        {
            //通过系统api获取当前线程的tid
            t_cachedTid = static_cast<int>(::syscall(SYS_gettid));
        }
    }
}