#include "TcpConnection.h"

#include <functional>
#include <thread>
#include <sstream>
#include <errno.h>
#include "../../base/Platform.h"
#include "../../deng.h"

using namespace net;

//dneg; 已经连接上客户端后调用
void net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_DEBUG("%s -> is %s",
        conn->localAddress().toIpPort().c_str(),
        conn->peerAddress().toIpPort().c_str(),
        (conn->connected() ? "UP" : "DOWN"));
    // do not call conn->forceClose(), because some users want to register message callback only.
}


//degn: 读取到客户端数据后调用
void net::defaultMessageCallback(const TcpConnectionPtr&, ByteBuffer* buf, Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : m_loop(loop),
    m_name(nameArg),
    m_state(kConnecting),
    m_socket(new Socket(sockfd)),
    m_channel(new Channel(loop, sockfd)),
    m_localAddr(localAddr),
    m_peerAddr(peerAddr),
    m_highWaterMark(64 * 1024 * 1024)
{
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_DEBUG("TcpConnection::TcpConnection 构造, name = %s ，地址 =  0x%x fd=%d", m_name.c_str(), this, sockfd);
    m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::析构 , name =%s ，地址 =  0x%x , fd=%d ,state=%s",
        m_name.c_str(), this, m_channel->fd(), stateToString());
    //assert(state_ == kDisconnected);
}

void TcpConnection::send(const void* data, int len)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            string message(static_cast<const char*>(data), len);
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
                    this,     // FIXME
                    message));
        }
    }
}

void TcpConnection::send(const string& message)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
                    this,     // FIXME
                    message));
            //std::forward<string>(message)));
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::send(ByteBuffer* buf)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string&)>(&TcpConnection::sendInLoop),
                    this,     // FIXME
                    buf->retrieveAllAsString()));
            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const string& message)
{
    sendInLoop(message.c_str(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    m_loop->assertInLoopThread();
    int32_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (m_state == kDisconnected)
    {
        LOG_DEBUG("TcpConnection::sendInLoop, disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = sockets::write(m_channel->fd(), data, len);
        //TODO: 打印threadid用于调试，后面去掉
        //std::stringstream ss;
        //ss << std::this_thread::get_id();
        //LOGI << "send data in threadID = " << ss;

        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && m_writeCompleteCallback)
            {
                m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop,发送数据给客户端出错");
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    //assert(remaining <= len);
    if (remaining > len)
        return;

    //deng; 如果没有发送出错，把剩余还没有发送的内容添加到发送缓冲区里。
    if (!faultError && remaining > 0)
    {
        size_t oldLen = m_outputBuffer.readableBytes();

        //dneg; 这个回调暂时没有用过
        if (oldLen + remaining >= m_highWaterMark
            && oldLen < m_highWaterMark
            && m_highWaterMarkCallback)
        {
            m_loop->queueInLoop(std::bind(m_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
        }

        //deng; 把剩余内容添加到发送缓冲区
        m_outputBuffer.append(static_cast<const char*>(data) + nwrote, remaining);

        //deng: 设置写事件
        if (!m_channel->isWriting())
        {
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (m_state == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    m_loop->assertInLoopThread();
    if (!m_channel->isWriting())
    {
        // we are not writing
        m_socket->shutdownWrite();
    }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

//dneg: 给session 层来调用关闭连接的。
void TcpConnection::forceClose()
{
    LOG_DEBUG_NET("TcpConnection::forceClose()调用 ,session关闭连接,client = %s, fd = %d", m_peerAddr.toIpPort().c_str(), m_socket->fd());
    // FIXME: use compare and swap
    if (m_state == kConnected || m_state == kDisconnecting)
    {
        setState(kDisconnecting);
        m_loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}


void TcpConnection::forceCloseInLoop()
{
    m_loop->assertInLoopThread();
    LOG_DEBUG_NET("TcpConnection::forceCloseInLoop()调用 ,client = %s, fd = %d", m_peerAddr.toIpPort().c_str(), m_socket->fd());

    if (m_state == kConnected || m_state == kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char* TcpConnection::stateToString() const
{
    switch (m_state)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    m_socket->setTcpNoDelay(on);
}


//deng: 设置客户端的读事件， 并调用连接完成的回调函数
void TcpConnection::connectEstablished()
{
    m_loop->assertInLoopThread();
    if (m_state != kConnecting)
    {
        //一定不能走这个分支
        return;
    }

    setState(kConnected);

    //假如正在执行这行代码时，对端关闭了连接
    if (!m_channel->enableReading())
    {
        LOG_DEBUG("TcpConnection::connectEstablished(), enableReading failed. 可能注册读事件的时候客户顿关闭了连接");
        //setState(kDisconnected);
        handleClose();
        return;
    }

    //connectionCallback_指向void XXServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
    m_connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnected)
    {
        setState(kDisconnected);
        m_channel->disableAll();

        m_connectionCallback(shared_from_this());
    }
    //deng: 底层设计有点问题，设计好的话  m_channel->disableAll(), 调用就可以了，这个应该是多余的才对。
    m_channel->remove();
}

//deng: 最终调用 sockets::readv 函数 ，读取客户端的数据到缓冲区。并通知到应用层读取到数据。
void TcpConnection::handleRead(Timestamp receiveTime)
{
    m_loop->assertInLoopThread();
    int savedErrno = 0;
    int32_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno);
    if (n > 0)
    {
        //messageCallback_指向CTcpSession::OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receiveTime)
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if (n == 0)//deng: 客户端断开连接
    {
        LOG_DEBUG("TcpConnection::handleRead : 客户端断开连接，导致读取数据为 n == 0 ");
        handleClose();
    }
    else
    {
        errno = savedErrno;
        // LOGSYSE("TcpConnection::handleRead");
        LOG_ERROR("TcpConnection::handleRead : 读客户端数据出错 ");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    m_loop->assertInLoopThread();
    if (m_channel->isWriting())
    {
        int32_t n = sockets::write(m_channel->fd(), m_outputBuffer.peek(), m_outputBuffer.readableBytes());
        if (n > 0)
        {
            m_outputBuffer.retrieve(n);
            if (m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if (m_writeCompleteCallback)
                {
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
                }

                //deng: 这一步有点疑惑？？？
                if (m_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleRead : 往客户端 写数据 出错 ");
            // LOGSYSE("TcpConnection::handleWrite");
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
            //added by zhangyl 2019.05.06
            handleClose();
        }
    }
    else
    {
        LOGD("Connection fd = %d  is down, no more writing", m_channel->fd());
    }
}

void TcpConnection::handleClose()
{
    //在Linux上当一个链接出了问题，会同时触发handleError和handleClose
    //为了避免重复关闭链接，这里判断下当前状态
    //已经关闭了，直接返回
    if (m_state == kDisconnected)
        return;

    m_loop->assertInLoopThread();

    LOG_DEBUG_NET("TcpConnection::handleClose()调用 ,client = %s, fd = %d, state = %s", m_peerAddr.toIpPort().c_str(), m_socket->fd(), stateToString());
    // LOGD("fd = %d  state = %s", m_channel->fd(), stateToString());
    //assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    m_connectionCallback(guardThis);

    // must be the last line
    //deng: 这个回调没有用，按理来说 m_connectionCallback 处理连接，m_closeCallback 这个处理关闭（断开连接）。实际上都是 m_connectionCallback在处理
    m_closeCallback(guardThis); 

    //只处理业务上的关闭，真正的socket fd在TcpConnection析构函数中关闭
    //if (socket_)
    //{
    //    sockets::close(socket_->fd());
    //}
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(m_channel->fd());
    // LOGE("TcpConnection::%s handleError [%d] - SO_ERROR = %s", m_name.c_str(), err, strerror(err));
    LOG_ERROR("TcpConnection::handleError()调用 ,client = %s, fd = %d, state = %s", m_peerAddr.toIpPort().c_str(), m_socket->fd(), stateToString());
    LOG_ERROR("错误信息::%s handleError [%d] - SO_ERROR = %s", m_name.c_str(), err, strerror(err));

    //调用handleClose()关闭连接，回收Channel和fd
    handleClose();
}
