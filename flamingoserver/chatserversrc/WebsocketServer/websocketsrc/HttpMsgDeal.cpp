
#include "HttpMsgDeal.h"
#include "../../../chatserversrc/UserManager.h"

std::map<std::string, LoginUserInfo> HttpMsgDeal::LoginUserInfo_map;

void HttpMsgDeal::onLoginResponse(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn)
{
    Json::Value jsonRoot = msgInfo.getMsgJosnValue();
    if (!jsonRoot[1]["username"].isString() || !jsonRoot[1]["password"].isString() || !jsonRoot[1]["clienttype"].isInt() || !jsonRoot[1]["status"].isInt())
    {
        LOG_DEBUG_WS("invalid json: %s, client: %s", msgInfo.getMsg().c_str(), conn->peerAddress().toIpPort().c_str());
        return;
    }

    string username = jsonRoot[1]["username"].asString();
    string password = jsonRoot[1]["password"].asString();
    int clientType = jsonRoot[1]["clienttype"].asInt();

    std::ostringstream os;
    User cachedUser;
    cachedUser.userid = 0;
    Singleton<UserManager>::Instance().getUserInfoByUsername(username, cachedUser);

    if (cachedUser.userid == 0)
    {
        // TODO: 这些硬编码的字符应该统一放到某个地方统一管理
        os << "{\"code\": 102, \"msg\": \"not registered\"}";
        LOG_DEBUG_WS("用户登录失败， 还没注册， client = %s", conn->peerAddress().toIpPort().c_str());
    }
    else
    {
        if (cachedUser.password != password)
        {
            os << "{\"code\": 103, \"msg\": \"incorrect password\"}";
            LOG_DEBUG_WS("用户登录密码错误， client = %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            //记录用户信息
            m_userinfo.userid = cachedUser.userid;
            m_userinfo.username = username;
            m_userinfo.nickname = cachedUser.nickname;
            m_userinfo.password = password;
            m_userinfo.clienttype = jsonRoot[1]["clienttype"].asInt();
            m_userinfo.status = jsonRoot[1]["status"].asInt();

            //记录登录信息到一个静态数组
            LoginUserInfo loginUserInfo;
            loginUserInfo.m_userinfo = m_userinfo;
            loginUserInfo.clientipAndPort = conn->peerAddress().toIpPort();
            // LOG_DEBUG_WS("*****************\r\n");

            // cout << "begin" << endl;
            // cout << conn->peerAddress().toIp().c_str() << endl;
            // cout << "end" << endl;

            // LOG_DEBUG_WS("*****************\r\n");
            // LOG_DEBUG_WS("conn->peerAddress().toIp() = %s", conn->peerAddress().toIp().c_str());
            // LOG_DEBUG_WS("conn->peerAddress().toIpPort() = %s", conn->peerAddress().toIpPort().c_str());
            // LOG_DEBUG_WS("conn->peerAddress().toPort() = %d", conn->peerAddress().toPort());
            // LOG_DEBUG_WS("*****************\r\n");

            LoginUserInfo_map[conn->peerAddress().toIp()] = loginUserInfo;
            LOG_DEBUG_WS("HttpMsgDeal::LoginUserInfo_map.size() = %d", HttpMsgDeal::LoginUserInfo_map.size());

            os << "{\"code\": 0, \"msg\": \"ok\", \"userid\": " << m_userinfo.userid << ",\"username\":\"" << cachedUser.username << "\", \"nickname\":\""
               << cachedUser.nickname << "\", \"facetype\": " << cachedUser.facetype << ", \"customface\":\"" << cachedUser.customface << "\", \"gender\":" << cachedUser.gender
               << ", \"birthday\":" << cachedUser.birthday << ", \"signature\":\"" << cachedUser.signature << "\", \"address\": \"" << cachedUser.address
               << "\", \"phonenumber\": \"" << cachedUser.phonenumber << "\", \"mail\":\"" << cachedUser.mail << "\"}";
        }
    }

    //登录信息应答
    m_sengMsg.makeMsg(msg_type_login, m_seq, os.str());

    // LOG_DEBUG_WS("Response to client: cmd=msg_type_login, data=%s, userid=", os.str().c_str(), m_userinfo.userid);
    LOG_DEBUG_WS("用户登录成功， client = %s", conn->peerAddress().toIpPort().c_str());
    // LOG_DEBUG_WS("用户信息： userid = %d, username =  %s, password = %s, nickname: %s",
    //              m_userinfo.userid, m_userinfo.username.c_str(), m_userinfo.password.c_str(), m_userinfo.nickname.c_str());
}

// deng; 注册只需要 用户名 + 昵称 + 密码；就可以了。
void HttpMsgDeal::registerUser(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn, bool keepalive, std::string &retData)
{

    Json::Value jsonRoot = msgInfo.getMsgJosnValue();

    // deng; 必须有 用户名，昵称，密码。
    if (!jsonRoot[1]["username"].isString() || !jsonRoot[1]["nickname"].isString() || !jsonRoot[1]["password"].isString())
    {
        LOG_DEBUG_WS("BussinessLogic::registerUser: 注册信息不正确，invalid json =  %s, client = %s", msgInfo.getMsg().c_str(), conn->peerAddress().toIpPort().c_str());
        // LOGW("invalid json: %s, client: %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        return;
    }

    User u;
    u.username = jsonRoot[1]["username"].asString();
    u.nickname = jsonRoot[1]["nickname"].asString();
    u.password = jsonRoot[1]["password"].asString();

    // std::string retData;
    User cachedUser;
    cachedUser.userid = 0;
    Singleton<UserManager>::Instance().getUserInfoByUsername(u.username, cachedUser);

    if (cachedUser.userid != 0)
    {
        LOG_DEBUG("BussinessLogic::registerUser: 注册失败，用户已经存在");
        retData = "{\"code\": 101, \"msg\": \"registered already\"}";
    }
    else
    {
        if (!Singleton<UserManager>::Instance().addUser(u))
        {
            LOG_DEBUG("BussinessLogic::registerUser: 注册失败, addUser()函数调用失败");
            retData = "{\"code\": 100, \"msg\": \"register failed\"}";
        }

        else
        {
            LOG_DEBUG("BussinessLogic::registerUser: 注册成功");
            Singleton<UserManager>::Instance().printf_allUserInfo(); //打印一下所有注册的信息

            retData = "{\"code\": 0, \"msg\": \"ok\"}";
        }
    }
    m_sengMsg.makeMsg(msg_type_register, m_seq, retData);
    // LOGI << "Response to client: cmd=msg_type_register" << ", userid=" << u.userid << ", data=" << retData;
}
void HttpMsgDeal::onRegisterResponse(const MsgInfo &msgInfo, const std::shared_ptr<TcpConnection> &conn)
{
    string retData;
    registerUser(msgInfo, conn, true, retData);

    if (!retData.empty())
    {
        m_sengMsg.makeMsg(msg_type_register, m_seq, retData);
    }
}

bool HttpMsgDeal::msgDeal(const std::shared_ptr<TcpConnection> &conn, const MsgInfo &msgInfo)
{
    int32_t cmd = msgInfo.getMsgCmd();
    switch (cmd)
    {
    //注册
    case msg_type_register:
        onRegisterResponse(msgInfo, conn);
        break;
    //登录
    case msg_type_login:
        // LOG_DEBUG_WS("进入登录处理");
        onLoginResponse(msgInfo, conn);
        break;
    default:
        break;
    }
    return true;
}
