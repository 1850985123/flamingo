#include "ByteBuffer.h"

#include "../../base/Platform.h"

#include "../Callbacks.h"
#include "../../deng.h"

using namespace net;

const char ByteBuffer::kCRLF[] = "\r\n";

const size_t ByteBuffer::kCheapPrepend;
const size_t ByteBuffer::kInitialSize;

int32_t ByteBuffer::readFd(int fd, int* savedErrno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    const size_t writable = writableBytes();
#ifndef WIN32
    struct iovec vec[2];

    vec[0].iov_base = begin() + m_writerIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf; //deng: 缓冲区读满了，之后的内容存储的位置。
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this ByteBuffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1; // dneg: 感觉不需要这一步， readv 应该会先把  writable 读满，之后在读取到 extrabuf。
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
#else
    const int32_t n = sockets::read(fd, extrabuf, sizeof(extrabuf));
#endif
    if (n <= 0)
    {
#ifdef WIN32
        * savedErrno = ::WSAGetLastError();
#else
        * savedErrno = errno;
#endif
    }
    else if (size_t(n) <= writable)
    {
#ifdef WIN32
        //Windows平台需要手动把接收到的数据加入ByteBuffer中，Linux平台已经在 struct iovec 中指定了缓冲区写入位置
        append(extrabuf, n);
#else
        m_writerIndex += n;
#endif
    }
    else
    {
#ifdef WIN32
        //Windows平台直接将所有的字节放入缓冲区去
        append(extrabuf, n);
#else
        //Linux平台把剩下的字节补上去
        m_writerIndex = m_buffer.size();
        append(extrabuf, n - writable);
#endif
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    return n;
}
