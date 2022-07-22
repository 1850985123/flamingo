#pragma once

#include <memory>

#include "../Callbacks.h"

#include "../InetAddress.h"
#include "..//ByteBuffer/ByteBuffer.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace net
{
    class EventLoop;
    class Channel;
    class Socket;

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(EventLoop* loop,
            const string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr);
        ~TcpConnection();

        EventLoop* getLoop() const { return m_loop; }
        const string& name() const { return m_name; }
        const InetAddress& localAddress() const { return m_localAddr; }
        const InetAddress& peerAddress() const { return m_peerAddr; }
        bool connected() const { return m_state == kConnected; }

        void send(const void* message, int len);
        void send(const string& message);
        void send(ByteBuffer* message);  // this one will swap data
        void shutdown();
        void forceClose();

        void setTcpNoDelay(bool on);

        void setConnectionCallback(const ConnectionCallback& cb)
        {
            m_connectionCallback = cb;
        }

        void setMessageCallback(const MessageCallback& cb)
        {
            m_messageCallback = cb;
        }

        //设置成功发完数据执行的回调
        void setWriteCompleteCallback(const WriteCompleteCallback& cb)
        {
            m_writeCompleteCallback = cb;
        }
            // Internal use only.
        void setCloseCallback(const CloseCallback& cb)
        {
            m_closeCallback = cb;
        }



        void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
        {
            m_highWaterMarkCallback = cb; m_highWaterMark = highWaterMark;
        }

        ByteBuffer* inputBuffer()
        {
            return &m_inputBuffer;
        }

        ByteBuffer* outputBuffer()
        {
            return &m_outputBuffer;
        }



        void connectEstablished();
        void connectDestroyed();

    private:
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();
        // void sendInLoop(string&& message);
        void sendInLoop(const string& message);
        void sendInLoop(const void* message, size_t len);
        void shutdownInLoop();
        // void shutdownAndForceCloseInLoop(double seconds);
        void forceCloseInLoop();
        void setState(StateE s) { m_state = s; }
        const char* stateToString() const;

    private:
        EventLoop* m_loop;
        const string                m_name;
        StateE                      m_state;
        
        std::unique_ptr<Socket>     m_socket;
        std::unique_ptr<Channel>    m_channel;

        const InetAddress           m_localAddr;
        const InetAddress           m_peerAddr;

        ConnectionCallback          m_connectionCallback;   //dneg; 已经连接上客户端后调用
        MessageCallback             m_messageCallback;      //degn: 读取到客户端数据后调用, 这个地方的 ByteBuffer* 参数感觉不要也可以啊？
        WriteCompleteCallback       m_writeCompleteCallback;//deng: 有使用
        CloseCallback               m_closeCallback;        //deng: 有使用

        HighWaterMarkCallback       m_highWaterMarkCallback;//deng: 这个没有用过，暂时不知道用来干什么？
        

        size_t                      m_highWaterMark;
        ByteBuffer                  m_inputBuffer;
        ByteBuffer                  m_outputBuffer;
    };

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}
