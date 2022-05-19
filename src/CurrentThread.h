// 用来获取当前线程id
#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // __thread/thread_local 将全局变量定义到每个线程中，每个线程中都有一份。
    // 当前线程的tid
    extern thread_local int t_cachedTid;

    // 获取线程id
    void cacheTid();

    inline int tid()
    {
        // 如果还没有获取过线程的id
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }

} // namespace CurrentThread
