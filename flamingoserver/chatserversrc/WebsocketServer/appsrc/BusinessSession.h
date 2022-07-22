/***************************************************************
 * 版权所有 :
 * 文件名   : BusinessSession.h
 * 作者     : zhangyl
 * 版本     : 1.0.0.0
 * 创建日期 : 2019.03.29
 * 描述     :
 ***************************************************************/

#ifndef __BUSINESS_SESSION_H__
#define __BUSINESS_SESSION_H__

#include <string>

#include "../websocketsrc/MyWebSocketSession.h"

struct OnlineUserInfo;
/**
 * 使用方法：创建你自己的BusinessSession类，在这里专注于业务处理
 * BussinessSession类专注于业务处理，WebSocketSession类专注于网络通信本身
 */
class BusinessSession : public MyWebSocketSession
{
public:
    BusinessSession(std::shared_ptr<TcpConnection> &conn, int sessionid);
    virtual ~BusinessSession() = default;

public:
    void onConnect() override;
    bool onMessage(const std::string &strClientMsg) override;

private:
    void sendWelcomeMsg();
    void onChatResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn);
    void onFindUserResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn);
    void onOperateFriendResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn);

private:
    static std::string m_strWelcomeMsg; //欢迎信息

public:
    void onGetFriendListResponse(const std::shared_ptr<TcpConnection> &conn);
    void makeUpFriendListInfo(std::string &friendinfo, const std::shared_ptr<TcpConnection> &conn);

private:
    int32_t m_seq; //当前Session数据包序列号
};

#endif //!__BUSINESS_SESSION_H__
