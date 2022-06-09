#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据
 * Poller工作在LT模式，如果fd上的数据没有读完，那么会一直通知
 * Buffer缓冲区是有大小的，但是从fd上读数据的时候，是不知道tcp数据的最终大小怎么办？
 */
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存,使用完后自动销毁
    // iovec是供readv和writev使用的结构体,readv和writev可以从多个缓冲区中读写数据
    struct iovec vec[2];
    const size_t writable = writableBytes(); // buffer中剩余可写空间的大小
    // 第一块缓冲区
    vec[0].iov_base = begin() + writerIndex_; // iov_base 是iovec缓冲区的起始位置
    vec[0].iov_len = writable;                // iov_len 是iovec缓冲区的大小
    // 第二块缓冲区，如果buffer中位置不够，那么就将数据读到栈上的 extrabuf 中
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    /**
     * 计算使用的缓冲区的个数
     * 如果buffer中的空间 > 65536B = 64K, 那么说明buffer里有足够的空间，就不使用extrabuf
     * 如果buffer中不够64k，那么就需要选择extrabuf，这个时候一次最多可以读取 128k
     */
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    // 使用readv读数据
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        // 如果extrabuf里面也写入了数据，将extrabuf中的数据写入buffer
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        // 出错
        *saveErrno = errno;
    }
    return n;
}