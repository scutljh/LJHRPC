#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 从fd读
// 需要记录读取大小  不知道从网络中一次写多少，使用了readv
// read 本身读一次默认情况下是和系统缓冲区一致，<=64kb
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // stack buf 64kb  (一次最多读64k)
    char extrabuf[65536] = {0};

    // 搭配readv使用，能够同时填入使用此结构的多个指定的位置(多块区域可以不连续)
    struct iovec vec[2];
    // buffer剩余可写
    const size_t writable = writableBytes();

    // 使用readv填，如果第一个vec[0]够的话就填入完成
    // 如果第一个vec[0]不够，剩下的数据会以此类推填入后面的vec[1]
    // 直到填完，或者vec用光
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 当此处vec[1]有数据时，再全搬到缓冲区中，加上我之前的扩容机制
    // 达到0空间浪费

    // 如果剩余可写小于64kb，那么需要两份，如果大于了，则只需要一份(一次最多读64kb)
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

    const ssize_t n = ::readv(fd, vec, iovcnt); // n:读取的字节数
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        // 已写入，直接调整
        writerIndex_ += n;
    }
    else // 两个vec都写入了数据,extra也有数据，写入
    {
        // 对extrabuf进行扩容
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 原来的写缓冲区已经写满了
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    //outputbuffer.
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}