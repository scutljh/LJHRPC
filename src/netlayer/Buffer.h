#pragma once
#include "noncopyable.h"

#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>

// 定义缓冲区 --aim--> 非阻塞IO非常有必要用  还能解决粘包问题
class Buffer
{
public:
    // 8字节头部长度
    static const size_t CheapPrepend = 8;
    // 缓冲区初始容量
    static const size_t InitalSize = 1024;

    explicit Buffer(size_t initalSize = InitalSize)
        : buffer_(CheapPrepend + InitalSize), readerIndex_(CheapPrepend), writerIndex_(CheapPrepend)
    {
    }

    // 各段长度
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    // 对缓冲区的标记进行重定位操作
    void retrieve(size_t len)
    {
        if (len < readableBytes()) // 可读缓冲区还有数据没读完
        {
            readerIndex_ += len;
        }
        else // 读完了就复位
        {
            retrieveAll();
        }
    }
    void retrieveAll()
    {
        readerIndex_ = CheapPrepend;
        writerIndex_ = CheapPrepend;
    }
    // 将buffer中全部数据提取并转为string
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        // 拿到buffer中可读区段readable bytes这一块的全部数据
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    //往buffer中写
    void append(const char* data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        //written
        writerIndex_+=len;
    }

    //从fd上读取数据，直接封装到buffer里
    ssize_t readFd(int fd,int* saveErrno);
    //..
    ssize_t writeFd(int fd,int* saveErrno);

    //  写入缓冲区
    void ensureWriteableBytes(size_t len)
    {
        // 不够写入，需要扩容
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    //定位
    char* beginWrite()
    {
        return begin()+writerIndex_;
    }
    const char* beginWrite() const
    {
        return begin()+writerIndex_;
    }
    

    ~Buffer(){}

private:
    // 需要源指针，因为socket相关需要用
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        // 当前可写的空间+前面空闲的 仍然不够 我要求的大小（现在必须写入len长度）  （两边都加了头，没问题，必须有一个头）
        if (writableBytes() + prependableBytes() < len + CheapPrepend)
        {
            // 只能扩容buffer  保证能把这次写入
            buffer_.resize(writerIndex_ + len);
        }
        else // 使用空闲的
        {
            size_t readable = readableBytes();
            // 未读的移动到前面去
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + CheapPrepend);

            readerIndex_ = CheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_; // 为了扩容方便
    size_t readerIndex_;
    size_t writerIndex_;
};
