/**
 * WebSocket会话类, MyWebSocketSession.h
 * zhangyl 2019.08.28
 */
#ifndef __MY_WEBSOCKET_SESSION_H__
#define __MY_WEBSOCKET_SESSION_H__

#include <string>
#include <map>
#include <memory>

#include "../../../deng.h"
#include "HttpParse.h"
#include "httpMsg.h"
using namespace net;

// 4 bits
enum MyOpCode : unsigned char
{
    CONTINUATION_FRAME = 0x0,
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    RESERVED1 = 0x3,
    RESERVED2 = 0x4,
    RESERVED3 = 0x5,
    RESERVED4 = 0x6,
    RESERVED5 = 0x7,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA,
    RESERVED6 = 0xB,
    RESERVED7 = 0xC,
    RESERVED8 = 0xD,
    RESERVED9 = 0xE,
    RESERVED10 = 0xF
};

class MyWebSocketSession
{
public:
    MyWebSocketSession(std::shared_ptr<TcpConnection> &conn, int sessionid);
    virtual ~MyWebSocketSession() = default;

    MyWebSocketSession(const MyWebSocketSession &rhs) = delete;
    MyWebSocketSession &operator=(const MyWebSocketSession &rhs) = delete;

public:
    const char *getClientInfo() const
    {
        return m_strClientInfo.c_str();
    }

    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (m_tmpConn.expired())
            return NULL;

        return m_tmpConn.lock();
    }

    std::string getHeader(const char *headerField) const;
    std::string getHeaderIgnoreCase(const char *headerField) const;

    void close();

    void send(const std::string &data, int32_t opcode = MyOpCode::TEXT_FRAME, bool compress = false);
    void send(const char *data, size_t dataLength, int32_t opcode = MyOpCode::TEXT_FRAME, bool compress = false);
    void sendAndClose(const char *data, size_t dataLength, bool compress = false);

    //子类按需改写
    virtual void onConnect()
    {
    }

    //子类按需改写
    virtual void onDisconnect()
    {
    }

    //子类改写该函数处理业务
    virtual bool onMessage(const std::string &strClientMsg)
    {
        return false;
    }

    //子类改写该函数处理业务
    virtual void onPing();

    //子类改写该函数处理业务
    virtual void onPong()
    {
    }

    //有数据可读, 会被多个工作loop调用
    void onRead(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer, Timestamp receivTime);

private:
    bool handleHandshake(const std::string &data, const std::shared_ptr<TcpConnection> &conn);
    bool decodePackage(ByteBuffer *pBuffer, const std::shared_ptr<TcpConnection> &conn);
    bool parseHttpPath(const std::string &str);
    void makeUpgradeResponse(const char *secWebSocketAccept, std::string &response);
    void unmaskData(std::string &src, const char *maskingKey);
    bool processPackage(const std::string &data, MyOpCode opcode, const std::shared_ptr<TcpConnection> &conn);

    void sendPackage(const char *data, size_t length);

public:
    int32_t getUserId()
    {
        return m_userinfo.userid;
    }
    int32_t getClientType()
    {
        return m_userinfo.clienttype;
    }

protected:
    std::string m_strClientInfo;

private:
    //是否已经升级为http协议
    bool m_bUpdateToWebSocket;
    std::weak_ptr<TcpConnection> m_tmpConn;

    //缓存的数据
    std::string m_strParsedData;

    //客户端是否要求压缩
    bool m_bClientCompressed;

    HttpParse m_httpParse;

private:
    int32_t m_id; // session id
protected:
    OnlineUserInfo m_userinfo; // deng; 这些信息是在用户登录的时候记录的。通过 usermane在用户表中找到用户信息，在赋值给 m_userinfo
};

#endif //!__MY_WEBSOCKET_SESSION_H__