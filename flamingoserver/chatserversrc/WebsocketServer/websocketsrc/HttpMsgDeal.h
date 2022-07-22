#pragma once

#include "../../Msg.h"
#include "httpMsg.h"
#include <map>
#include "../../../deng.h"
#include "../../ChatSession.h"

// struct OnlineUserInfo
// {
//     int32_t userid; // deng: 第几个注册的用户，userid就是几
//     std::string username;
//     std::string nickname;
//     std::string password;
//     int32_t clienttype; //客户端类型, 0未知, pc=1, android/ios=2
//     int32_t status;     //在线状态 0离线 1在线 2忙碌 3离开 4隐身
// };

struct LoginUserInfo
{
    OnlineUserInfo m_userinfo;
    string clientipAndPort;
};

// class TcpConnection;
// class MsgInfo;

class HttpMsgDeal
{
public:
    static std::map<std::string, LoginUserInfo> LoginUserInfo_map;

public:
    HttpMsgDeal() = default;
    ~HttpMsgDeal() = default;

public:
    bool msgDeal(const std::shared_ptr<TcpConnection> &conn, const MsgInfo &msgInfo);
    inline MsgInfo getSengMsg() { return m_sengMsg; };

private:
    void registerUser(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn, bool keepalive, std::string &retData);
    void onRegisterResponse(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn);
    void onLoginResponse(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn);

private:
    int32_t m_seq; //当前Session数据包序列号
    MsgInfo m_sengMsg;
    OnlineUserInfo m_userinfo; // deng; 这些信息是在用户登录的时候记录的。通过 usermane在用户表中找到用户信息，在赋值给 m_userinfo
};
