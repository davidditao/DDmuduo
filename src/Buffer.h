#pragma once

#include <vector>
#include <string>
#include <algorithm>

/**
 * 参照netty实现的非阻塞io的缓冲区
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 */

class Buffer
{
public:
    /**
     * buffer前面预留了 8 字节的空间（prepend）
     * 这样在序列化的时候可以便宜地在首部添加几个字节，而不必腾挪整个空间
     */
    static const size_t kCheapPrepend = 8;   
    static const size_t kInitialSize = 1024; // 数据部分的长度(prepend之后)

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }

    // 可读部分长度
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    // 可写部分长度
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // 返回read指针前面部分的长度
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    /**
     * 把onMessage函数上报的Buffer数据，转成string类型返回
     */
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        // 将可读部分转为字符串
        std::string result(peek(), len);
        // 上面一句已经把缓冲区中可读的数据读取出来了，这里要更新指针的位置
        retrieve(len);
        return result;
    }

    // 更新指针位置
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // 如果没读完，更新read指针
            readerIndex_ += len;
        }
        else
        {
            // len == readableBytes() 如果读完了
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        // 如果缓冲区中的数据都读完了，那就将所有的指针复位
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 使缓冲区可写数据(位置不够就需要扩容)
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            // 如果需要写的长度比可写部分长，那么需要扩容
            makeSpace(len);
        }
    }

    // 添加数据(写)
    void append(const char *data, size_t len)
    {
        // 保证可以写
        ensureWriteableBytes(len);
        // 将data写入缓冲区
        std::copy(data, data + len, beginWrite());
        // 更新写指针
        writerIndex_ += len;
    }

    // 返回可写部分的指针
    char *beginWrite()
    {
        return begin() + writerIndex_;
    }
    // 重载const版本
    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

private:
    // 获取vector的裸指针
    // 首元素的地址
    char *begin()
    {
        return &*buffer_.begin();
    }
    // 重载const版本
    const char *begin() const
    {
        return &*buffer_.begin();
    }

    // 扩容
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            // 如果buffer中所有空闲的位置都写不下了，直接扩容
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // 如果前面还有空闲的位置，那就将可读数据往前移
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_; // 缓冲区，使用vector可以更方便扩容（妙哇！）
    size_t readerIndex_;       // 可读区指针
    size_t writerIndex_;       // 可写区指针
};