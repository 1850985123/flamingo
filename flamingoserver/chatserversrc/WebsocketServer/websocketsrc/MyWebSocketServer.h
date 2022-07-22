/**
 * WebSocketServer类，MyWebSocketServer.h
 * zhangyl 2019.08.28
 */
#ifndef __MY_WEBSOCKET_SERVER_H__
#define __MY_WEBSOCKET_SERVER_H__

// #include "MyWebSocketServer.h"

#include <memory>
#include <mutex>
#include <thread>
#include <list>
#include "../../../deng.h"

using namespace net;

class BusinessSession;

class MyWebSocketServer final
{
public:
    MyWebSocketServer();
    ~MyWebSocketServer() = default;
    MyWebSocketServer(const MyWebSocketServer &rhs) = delete;
    MyWebSocketServer &operator=(const MyWebSocketServer &rhs) = delete;

public:
    bool init(const char *ip, short port, EventLoop *loop);
    void uninit();

    //新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
    void onConnection(std::shared_ptr<TcpConnection> conn);
    //连接断开
    void onClose(const std::shared_ptr<TcpConnection> &conn);

public:
    bool getSessionByUserIdAndClientType(std::shared_ptr<BusinessSession> &session, int32_t userid, int32_t clientType);
    bool getSessionsByUserId(std::list<std::shared_ptr<BusinessSession>> &sessions, int32_t userid);
    int32_t getUserClientTypeByUserId(int32_t userid);
    int32_t getUserStatusByUserId(int32_t userid);

private:
    std::shared_ptr<TcpServer> m_server;

    std::list<std::shared_ptr<BusinessSession>> m_sessions;
    std::mutex m_sessionMutex; //多线程之间保护m_sessions

    //开放的前置侦听ip地址
    std::string m_strWsHost;
    //开放的前置端口号
    int m_wsPort;

    std::atomic_int m_sessionId{}; // deng; 这个用来统计当前的session数量吧？？好像没有意义 m_sessions 的数量不就是吗
};

#endif //!__MY_WEBSOCKET_SERVER_H__