#pragma once

#include <atomic>
#include <map>
#include <memory>
#include "../InetAddress.h"
#include "../Callbacks.h"

namespace net
{
    class Acceptor;
    class EventLoop;
    class EventLoopThreadPool;

    class TcpServer
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;
        enum Option
        {
            kNoReusePort,
            kReusePort,   //deng; 解决 多个进程监听一个端口的惊群效应。
        };

        TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option = kReusePort);      //TODO: 默认修改成kReusePort
        ~TcpServer();

        const std::string& hostport() const { return m_hostport; }
        const std::string& name() const { return m_name; }
        EventLoop* getLoop() const { return m_loop; }
        ;
        void setThreadInitCallback(const ThreadInitCallback& cb)
        {
            m_threadInitCallback = cb;
        }

        void start(int workerThreadCount = 4);

        void stop();

        /// Set connection callback.
        /// Not thread safe.
        void setConnectionCallback(const ConnectionCallback& cb)
        {
            m_connectionCallback = cb;
        }

        /// Set message callback.
        /// Not thread safe.
        void setMessageCallback(const MessageCallback& cb)
        {
            m_messageCallback = cb;
        }

        /// Set write complete callback.
        /// Not thread safe.
        void setWriteCompleteCallback(const WriteCompleteCallback& cb)
        {
            m_writeCompleteCallback = cb;
        }

        void removeConnection(const TcpConnectionPtr& conn);

    private:
        /// Not thread safe, but in loop
        void newConnection(int sockfd, const InetAddress& peerAddr);
        /// Thread safe.

        /// Not thread safe, but in loop
        void removeConnectionInLoop(const TcpConnectionPtr& conn);

        typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    private:
        /*  dneg; 作用： ①绑定监听事件，②判断是否在本线程，③给线程池做参数*/
        EventLoop* m_loop;                              //deng: 主线程的loop
        const string                                    m_hostport;     // dneg: ip和端口
        const string                                    m_name;         // dnge: 服务器名字
        std::unique_ptr<EventLoopThreadPool>            m_eventLoopThreadPool;
        std::unique_ptr<Acceptor>                       m_acceptor;
        
        ConnectionCallback                              m_connectionCallback;       // deng: 新客户端连接事件
        MessageCallback                                 m_messageCallback;          //deng; 客户端的读事件
        WriteCompleteCallback                           m_writeCompleteCallback;    // deng: 客户端的写事件
        ThreadInitCallback                              m_threadInitCallback;       //dneg: 暂时没有用

        std::atomic<int>                                m_started;//deng: 服务器是否启动
        int                                             m_nextConnId;
        ConnectionMap                                   m_connections; //degn: 保存所有的已经创建的 TcpConnectionPtr
    };

}
