/**
 * MyWebSocketServer类，MyMyWebSocketServer.cpp
 * zhangyl 2019.08.28
 */

#include "MyWebSocketServer.h"
#include <string>
#include "../appsrc/BusinessSession.h"
#include "MyWebSocketSession.h"
#include "../../../deng.h"
enum CLIENT_TYPE
{
    CLIENT_TYPE_UNKOWN,
    CLIENT_TYPE_PC,
    CLIENT_TYPE_ANDROID,
    CLIENT_TYPE_IOS,
    CLIENT_TYPE_MAC
};

MyWebSocketServer::MyWebSocketServer()
{
}

bool MyWebSocketServer::init(const char *ip, short port, EventLoop *loop)
{
    InetAddress addr(ip, port);

    /* degn; （初始化阶段）一个服务器需要什么信息就可以了， ①服务器名字，②ip + 端口，③一个主loop,④需要的线程的数量，⑤客户端连接上的回调函数 */
    m_server.reset(new TcpServer(loop, addr, "MY-WEBSOCKET-SERVER", TcpServer::kReusePort));
    m_server->setConnectionCallback(std::bind(&MyWebSocketServer::onConnection, this, std::placeholders::_1));
    //启动侦听
    unsigned int threadCount = std::thread::hardware_concurrency() + 2;
    m_server->start(threadCount);

    LOG_DEBUG_WS("MyWebSocketServer::init, threadCount = ", threadCount);

    return true;
}

void MyWebSocketServer::uninit()
{
    if (m_server)
        m_server->stop();
}

//新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
void MyWebSocketServer::onConnection(std::shared_ptr<TcpConnection> conn)
{

    if (conn->connected())
    {
        LOG_DEBUG_WS("客户端连接: client = %s", conn->peerAddress().toIpPort().c_str());

        ++m_sessionId; // deng: 2022_7_9 好像意义不大
        std::shared_ptr<BusinessSession> spSession(new BusinessSession(conn, m_sessionId));
        /* deng:    主线程监听到客户端连接后，
                    Acceptor 设置的连接成功回调TcpServer::newConnection被调用 ，
                    TcpServer::newConnection 函数会把连接成功的客户端分配给不同的loop处理，
                    通过执行绑定TcpConnection::connectEstablished函数进行处理，
                    TcpConnection::connectEstablished里调用TcpConnection 里的m_connectionCallback回调。
                    也就是TcpServer 里的m_connectionCallback回调
                    */
        conn->setMessageCallback(std::bind(&MyWebSocketSession::onRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        {
            std::lock_guard<std::mutex> guard(m_sessionMutex);
            m_sessions.push_back(spSession);
        }
    }
    else
    {
        onClose(conn);
    }
}

//连接断开
void MyWebSocketServer::onClose(const std::shared_ptr<TcpConnection> &conn)
{
    // TODO: 这样的代码逻辑太混乱，需要优化
    std::unique_lock<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
    {
        //通过比对connection对象找到对应的session
        if ((*iter)->getConnectionPtr() == conn)
        {
            (*iter)->onDisconnect();
            LOG_DEBUG_WS("客户端连接: client = %s, session: 0x%0x", (*iter)->getClientInfo(), (int64_t)((*iter).get()));

            m_sessions.erase(iter);
            return;
        }
    }

    LOG_ERROR_WS("Unable to find session, conn = 0x%llx\n", (int64_t)conn->getLoop());
}

bool MyWebSocketServer::getSessionByUserIdAndClientType(std::shared_ptr<BusinessSession> &session, int32_t userid, int32_t clientType)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    std::shared_ptr<BusinessSession> tmpSession;
    for (const auto &iter : m_sessions)
    {
        tmpSession = iter;
        if (iter->getUserId() == userid && iter->getClientType() == clientType)
        {
            session = tmpSession;
            return true;
        }
    }

    return false;
}

bool MyWebSocketServer::getSessionsByUserId(std::list<std::shared_ptr<BusinessSession>> &sessions, int32_t userid)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    std::shared_ptr<BusinessSession> tmpSession;
    for (const auto &iter : m_sessions)
    {
        tmpSession = iter;
        if (iter->getUserId() == userid)
        {
            sessions.push_back(tmpSession);
            return true;
        }
    }

    return false;
}
