#include "TcpServer.h"

#include <stdio.h>  // snprintf
#include <functional>

#include "../../base/Platform.h"
#include "../../deng.h"
#include "../../base/Singleton.h"

 
using namespace net;

TcpServer::TcpServer(EventLoop* loop,
    const InetAddress& listenAddr,
    const std::string& nameArg,
    Option option)
    : m_loop(loop),
    m_hostport(listenAddr.toIpPort()),
    m_name(nameArg),
    m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
    //threadPool_(new EventLoopThreadPool(loop, name_)),
    m_connectionCallback(defaultConnectionCallback),
    m_messageCallback(defaultMessageCallback),
    m_started(0),
    m_nextConnId(1)
{
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    m_loop->assertInLoopThread();
    LOGD("TcpServer::~TcpServer [%s] destructing", m_name.c_str());

    stop();
}

//void TcpServer::setThreadNum(int numThreads)
//{
//  assert(0 <= numThreads);
//  threadPool_->setThreadNum(numThreads);
//}

void TcpServer::start(int workerThreadCount/* = 4*/)
{
    if (m_started == 0)
    {
        m_eventLoopThreadPool.reset(new EventLoopThreadPool());
        m_eventLoopThreadPool->init(m_loop, workerThreadCount);
        m_eventLoopThreadPool->start();

        //threadPool_->start(threadInitCallback_);
        //assert(!acceptor_->listenning());

        // deng; 这里直接调用监听函数， 如果不在主线程则把监听连接事件放在主线程等待执行队列
        m_loop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
        m_started = 1;
    }
}

void TcpServer::stop()
{
    if (m_started == 0)
        return;

    for (ConnectionMap::iterator it = m_connections.begin(); it != m_connections.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;  // dneg: 这个操作引用计数会不会增加？？？？
        it->second.reset(); 
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn)); //deng: 这里 conn 的引用指数会增加吗？？？
        conn.reset(); // deng : 这个时候给他reset会不会有问题？？？
    }

    m_eventLoopThreadPool->stop();

    m_started = 0; //deng:感觉多余啊 
}

//deng: 底层 m_acceptor 有新的连接到来调用的回调函数 。sockfd ： 客户端的 fd。 peerAddr ：客户端的 地址。
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    m_loop->assertInLoopThread();

    //deng: 轮询方式往 loop集合中 添加客户端连接 ;
    EventLoop* ioLoop = m_eventLoopThreadPool->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", m_hostport.c_str(), m_nextConnId);
    ++m_nextConnId;
    string connName = m_name + buf;

    LOGD("TcpServer::newConnection [%s] - new connection [%s] from %s", m_name.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    //deng: 通过socket 可以得到对应的 本地 ip 和端口信息 ？我还不确定
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    m_connections[connName] = conn;// dneg: 这个操作引用计数会不会增加？？？？

    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
    
    //该线程分离完io事件后，立即调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    m_loop->assertInLoopThread();

    // LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s", m_name.c_str(), conn->name().c_str());
    LOG_DEBUG("TcpServer::removeConnectionInLoop 服务器 = %s - TcpConnection =  %s", m_name.c_str(), conn->name().c_str());
        
    size_t n = m_connections.erase(conn->name());
    //(void)n;
    //assert(n == 1);
    if (n != 1)
    {
        //出现这种情况，是TcpConneaction对象在创建过程中，对方就断开连接了。
        // LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s, connection does not exist.", m_name.c_str(), conn->name().c_str());
        LOG_DEBUG("TcpServer::removeConnectionInLoop 服务器 = %s - TcpConnection =  %s, TcpConnection does not exist, ", m_name.c_str(), conn->name().c_str());
        LOG_DEBUG("TcpConnection不存在原因: 是TcpConneaction对象在创建过程中, 对方就断开连接了");
        
        return;
    }

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
