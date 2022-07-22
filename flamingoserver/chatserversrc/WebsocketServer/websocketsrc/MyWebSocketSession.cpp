/**
 * WebSocket会话类, MyWebSocketSession.cpp
 * zhangyl 2017.03.09
 */

#include <sstream>
#include <list>

#ifndef WIN32
#include <strings.h>
#endif

#include "MyWebSocketSession.h"

#include "../../../chatserversrc/UserManager.h"
#include "MyWebSocketServer.h"
#include "../../MsgCacheManager.h"
#include "../appsrc/BusinessSession.h"
// client端最大允许http包50M
#define MAX_WEBSOCKET_CLIENT_PACKAGE_LENGTH 50 * 1024 * 1024
//最小websocket包头大小
#define MIN_WEBSOCKET_PACKAGE_HEADER_LENGTH 6

//服务器端分包的大小，10M
#define MAX_WEBSOCKET_SERVER_PACKAGE_LENGTH 10 * 1024 * 1024

MyWebSocketSession::MyWebSocketSession(std::shared_ptr<TcpConnection> &conn, int sessionid) : m_id(sessionid), m_bUpdateToWebSocket(false), m_tmpConn(conn), m_bClientCompressed(false)
{
    m_strClientInfo = conn->peerAddress().toIpPort();
}

void MyWebSocketSession::onRead(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer, Timestamp receivTime)
{
    while (true)
    {
        /**
        GET /realtime HTTP/1.1
        Host: 127.0.0.1:9989
        Connection: Upgrade
        Pragma: no-cache
        Cache-Control: no-cache
        User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.100 Safari/537.36
        Upgrade: websocket
        Origin: http://coolaf.com
        Sec-WebSocket-Version: 13
        Accept-Encoding: gzip, deflate, br
        Accept-Language: zh-CN,zh;q=0.9,en;q=0.8
        Sec-WebSocket-Key: IqcAWodjyPDJuhGgZwkpKg==
        Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
       */
        size_t readableBytesCount = pBuffer->readableBytes();
        if (readableBytesCount >= MAX_WEBSOCKET_CLIENT_PACKAGE_LENGTH)
        {
            this->close();
            return;
        }

        if (!m_bUpdateToWebSocket)
        {
            HttpParse::PARSE_RESULT res = m_httpParse.parse_start(conn, pBuffer);
            switch (res)
            {
            case HttpParse::PACKAGE_NOT_ENOUGH:
                LOG_DEBUG_WS("HttpParse::PACKAGE_NOT_ENOUGH");
                break;
            case HttpParse::PACKAGE_ERROR:
                LOG_DEBUG_WS("HttpParse::PACKAGE_ERROR");
                conn->forceClose();
                break;
            case HttpParse::UPGRADE_TO_WEBSOCE:
                LOG_DEBUG_WS("HttpParse::UPGRADE_TO_WEBSOCE");
                conn->send(m_httpParse.getRespondContent());

                // deng:升级协议后需要知道的内容： 一、websocket数据通讯是否需要压缩，二、登录的用户信息
                m_bClientCompressed = m_httpParse.getClientCompressed();
                m_userinfo = m_httpParse.getLoginUserInfo();

                m_bUpdateToWebSocket = true;
                //实际调用子类改写的类
                onConnect();
                break;
            case HttpParse::PARSE_SUCCEED:
                LOG_DEBUG_WS("HttpParse::PARSE_SUCCEED");
                // LOG_DEBUG_WS("要发送的内容 = %s", m_httpParse.getRespondContent().c_str());
                // LOG_INFO_WS("响应消息的大小：%d, 内容 = \r\n%s", m_httpParse.getRespondContent().size(), m_httpParse.getRespondContent().c_str());
                // LOG_INFO_WS("响应消息的httpHeader = \r\n%s", m_httpParse.getRespondHttpHeader().c_str());
                // LOG_INFO_WS("响应消息的httpContent = \r\n%s", m_httpParse.getRespondHttpcontent().c_str());

                // LOG_INFO_WS("响应消息的大小：%d", m_httpParse.getRespondContent().size());
                conn->send(m_httpParse.getRespondContent());
                break;
            default:
                break;
            }
        }
        else
        {
            if (readableBytesCount < MIN_WEBSOCKET_PACKAGE_HEADER_LENGTH)
                return;

            if (!decodePackage(pBuffer, conn))
                conn->forceClose();
        }
        //不管包是否处理出错还是因为包数据不足都应该退出循环
        return;
    }

    //处理正常数据交流
}

bool MyWebSocketSession::decodePackage(ByteBuffer *pBuffer, const std::shared_ptr<TcpConnection> &conn)
{
    // WEBSOCKET RFC 文档：https://www.rfc-editor.org/rfc/rfc6455.txt
    /*
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+

     */
    size_t readableBytesCount = pBuffer->readableBytes();

    const int32_t TWO_FLAG_BYTES = 2;

    //最大包头长度
    const int32_t MAX_HEADER_LENGTH = 14;
    char pBytes[MAX_HEADER_LENGTH] = {0};
    //已经收到的数据大于最大包长时仅拷贝可能是包头的最大部分
    if (readableBytesCount > MAX_HEADER_LENGTH)
        memcpy(pBytes, pBuffer->peek(), MAX_HEADER_LENGTH * sizeof(char));
    else
        memcpy(pBytes, pBuffer->peek(), readableBytesCount * sizeof(char));

    bool FIN = (pBytes[0] & 0x80);
    // TODO: 这里就不校验了，因为服务器和未知的客户端之间无约定
    // bool RSV1, RSV2, RSV3;
    //取第一个字节的低4位获取数据类型
    int32_t opcode = (int32_t)(pBytes[0] & 0xF);

    // if (!FIN && opcode != MyOpCode::CONTINUATION_FRAME)
    //{
    //     LOG_ERROR_WS("FIN did not match opcode, client: %s", conn->peerAddress().toIpPort().c_str());
    //     return false;
    // }

    // if (opcode == MyOpCode::CLOSE)
    //{
    //     LOG_ERROR_WS("receive CLOSE opcode, client: %s", conn->peerAddress().toIpPort().c_str());
    //     return false;
    // }

    //取第二个字节的最高位，理论上说客户端发给服务器的这个字段必须设置为1
    bool mask = ((pBytes[1] & 0x80));
    // if (!mask)
    //{
    //     LOG_ERROR_WS("invalid mask flag, client: %s", conn->peerAddress().toIpPort().c_str());
    //     return false;
    // }

    int32_t headerSize = 0;
    int64_t bodyLength = 0;
    //按mask标志加上四个字节的masking-key长度
    if (mask)
        headerSize += 4;

    //取第二个字节的低七位
    int32_t payloadLength = (int32_t)(pBytes[1] & 0x7F);
    if (payloadLength <= 0 && payloadLength > 127)
    {
        LOG_ERROR_WS("invalid payload length, payloadLength: %d, client: %s", payloadLength, conn->peerAddress().toIpPort().c_str());
        return false;
    }

    if (payloadLength > 0 && payloadLength <= 125)
    {
        headerSize += TWO_FLAG_BYTES;
        bodyLength = payloadLength;
    }
    else if (payloadLength == 126)
    {
        headerSize += TWO_FLAG_BYTES;
        headerSize += sizeof(short);

        if ((int32_t)readableBytesCount < headerSize)
            return true;

        short tmp;
        memcpy(&tmp, &pBytes[2], 2);
        int32_t extendedPayloadLength = ::ntohs(tmp);
        bodyLength = extendedPayloadLength;
        //包体长度不符合要求
        if (bodyLength < 126 || bodyLength > UINT16_MAX)
        {
            LOG_ERROR_WS("illegal extendedPayloadLength, extendedPayloadLength: %d, client: %s", bodyLength, conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }
    else if (payloadLength == 127)
    {
        headerSize += TWO_FLAG_BYTES;
        headerSize += sizeof(uint64_t);

        //包长度不够
        if ((int32_t)readableBytesCount < headerSize)
            return true;

        int64_t tmp;
        memcpy(&tmp, &pBytes[2], 8);
        int64_t extendedPayloadLength = ::ntohll(tmp);
        bodyLength = extendedPayloadLength;
        //包体长度不符合要求
        if (bodyLength <= UINT16_MAX)
        {
            LOG_ERROR_WS("illegal extendedPayloadLength, extendedPayloadLength: %lld, client: %s", bodyLength, conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    if ((int32_t)readableBytesCount < headerSize + bodyLength)
        return true;

    //取出包头
    pBuffer->retrieve(headerSize);
    std::string payloadData(pBuffer->peek(), bodyLength);
    //取出包体
    pBuffer->retrieve(bodyLength);

    if (mask)
    {
        char maskingKey[4] = {0};
        // headerSize - 4即masking-key的位置
        memcpy(maskingKey, pBytes + headerSize - 4, 4);
        unmaskData(payloadData, maskingKey);
    }

    if (FIN)
    {
        //最后一个分片，与之前的合并（如果有的话）后处理
        m_strParsedData.append(payloadData);
        //包处理出错
        if (!processPackage(m_strParsedData, (MyOpCode)opcode, conn))
            return false;

        m_strParsedData.clear();
    }
    else
    {
        //非最后一个分片，先缓存起来
        m_strParsedData.append(payloadData);
    }

    return true;
}

void MyWebSocketSession::close()
{
    if (m_tmpConn.expired())
        return;

    std::shared_ptr<TcpConnection> conn = m_tmpConn.lock();
    conn->forceClose();
}

void MyWebSocketSession::unmaskData(std::string &src, const char *maskingKey)
{
    /*
     *  掩码键（Masking-key）是由客户端挑选出来的 32 位的随机数。掩码操作不会影响数据载荷的长度。掩码、反掩码操作都采用如下算法：

        首先，假设：

        original-octet-i：为原始数据的第 i 字节。
        transformed-octet-i：为转换后的数据的第 i 字节。
        j：为i mod 4的结果。
        masking-key-octet-j：为 mask key 第 j 字节。
        算法描述为： original-octet-i 与 masking-key-octet-j 异或后，得到 transformed-octet-i。

        j  = i MOD 4
        transformed-octet-i = original-octet-i XOR masking-key-octet-j
     */
    char j;
    for (size_t n = 0; n < src.length(); ++n)
    {
        j = n % 4;
        src[n] = src[n] ^ maskingKey[j];
    }
}

bool MyWebSocketSession::processPackage(const std::string &data, MyOpCode opcode, const std::shared_ptr<TcpConnection> &conn)
{
    if (opcode == MyOpCode::CLOSE)
    {
        LOG_ERROR_WS("received CLOSE opcode, close session, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }
    else if (opcode == MyOpCode::PING)
    {
        onPing();
    }
    else if (opcode == MyOpCode::PONG)
    {
        onPong();
    }
    else if (opcode == MyOpCode::TEXT_FRAME || opcode == MyOpCode::BINARY_FRAME)
    {
        std::string out;
        if (m_bClientCompressed)
        {
            // LOG_DEBUG_WS("接收到的websocket消息 压缩了");
            if (!ZlibUtil::inflate(data, out))
            {
                LOG_ERROR_WS("uncompress failed, dataLength: %d, client: %s", data.length(), conn->peerAddress().toIpPort().c_str());
                return false;
            }
        }
        else
        {
            // LOG_DEBUG_WS("接收到的websocket消息 沒有压缩");
            out = data;
        }

        onMessage(out);
    }

    return true;
}

void MyWebSocketSession::send(const std::string &data, int32_t opcode /* = MyOpCode::TEXT_FRAME*/, bool compress /* = false*/)
{
    /*
          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-------+-+-------------+-------------------------------+
         |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
         |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
         |N|V|V|V|       |S|             |   (if payload len==126/127)   |
         | |1|2|3|       |K|             |                               |
         +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
         |     Extended payload length continued, if payload len == 127  |
         + - - - - - - - - - - - - - - - +-------------------------------+
         |                               |Masking-key, if MASK set to 1  |
         +-------------------------------+-------------------------------+
         | Masking-key (continued)       |          Payload Data         |
         +-------------------------------- - - - - - - - - - - - - - - - +
         :                     Payload Data continued ...                :
         + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
         |                     Payload Data continued ...                |
         +---------------------------------------------------------------+
     */

    size_t dataLength = data.length();

    LOG_DEBUG_WS("发送到websocket客户端消息 = %s ,client = %s", data.c_str(), getConnectionPtr()->peerAddress().toIpPort().c_str());
    std::string destbuf;
    if (m_bClientCompressed && dataLength > 0)
    {
        // LOG_DEBUG_WS("进入压缩");

        if (!ZlibUtil::deflate(data, destbuf))
        {
            LOG_ERROR_WS("compress buf error, data: %s", data.c_str());
            return;
        }
    }
    else
        destbuf = data;

    // LOG_DEBUG_WS("destbuf.length(): %d", destbuf.length());

    // LOG_DEBUG_WS("输出destbuf的内容 = %s.lenght = %d", destbuf.c_str(), destbuf.length());

    dataLength = destbuf.length();

    char firstTwoBytes[2] = {0};
    // FIN
    firstTwoBytes[0] |= 0x80;

    // if (!(flags & SND_CONTINUATION)) {
    // opcode
    firstTwoBytes[0] |= opcode;
    //}

    // TODO：这里为啥有这个标记？
    const char compressFlag = 0x40;
    if (m_bClientCompressed)
        firstTwoBytes[0] |= compressFlag;

    // mask = 0;
    //实际发送的数据包
    std::string actualSendData;

    if (dataLength < 126)
    {
        firstTwoBytes[1] = dataLength;
        actualSendData.append(firstTwoBytes, 2);
    }
    else if (dataLength <= UINT16_MAX) // 2字节无符号整数最大数值（65535）
    {
        firstTwoBytes[1] = 126;
        char extendedPlayloadLength[2] = {0};
        uint16_t tmp = ::htons(dataLength);
        memcpy(&extendedPlayloadLength, &tmp, 2);
        actualSendData.append(firstTwoBytes, 2);
        actualSendData.append(extendedPlayloadLength, 2);
    }
    else
    {
        firstTwoBytes[1] = 127;
        char extendedPlayloadLength[8] = {0};
        uint64_t tmp = ::htonll((uint64_t)dataLength);
        memcpy(&extendedPlayloadLength, &tmp, 8);
        actualSendData.append(firstTwoBytes, 2);
        actualSendData.append(extendedPlayloadLength, 8);
    }

    actualSendData.append(destbuf);

    sendPackage(actualSendData.c_str(), actualSendData.length());

    // LOG_DEBUG_BIN((unsigned char*)actualSendData.c_str(), actualSendData.length());
}

void MyWebSocketSession::send(const char *data, size_t dataLength, int32_t opcode /* = MyOpCode::TEXT_FRAME*/, bool compress /* = false*/)
{
    std::string str(data, dataLength);
    send(str, opcode, compress);
}

void MyWebSocketSession::sendAndClose(const char *data, size_t dataLength, bool compress /* = false*/)
{
    send(data, dataLength, MyOpCode::CLOSE, compress);
}

void MyWebSocketSession::onPing()
{
    send("", 0, MyOpCode::PONG, m_bClientCompressed);
}

void MyWebSocketSession::sendPackage(const char *data, size_t length)
{
    if (m_tmpConn.expired())
        return;

    std::shared_ptr<TcpConnection> conn = m_tmpConn.lock();
    conn->send(data, length);
}
