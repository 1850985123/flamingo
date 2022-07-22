/**
 * 即时通讯的业务逻辑都统一放在这里，BusinessLogic.cpp
 * zhangyl 2018.05.16
 */

#include "BussinessLogic.h"
#include <string>
#include "../jsoncpp1.9.0/json.h"

#include "../base/Singleton.h"
#include "ChatServer.h"
#include "UserManager.h"

//deng; 注册只需要 用户名 + 昵称 + 密码；就可以了。
void BussinessLogic::registerUser(const std::string& data, const std::shared_ptr<TcpConnection>& conn, bool keepalive, std::string& retData)
{
    //{ "user": "13917043329", "nickname" : "balloon", "password" : "123" }

    Json::CharReaderBuilder b;
    Json::CharReader* reader(b.newCharReader());
    Json::Value jsonRoot;
    JSONCPP_STRING errs;

    bool ok = reader->parse(data.c_str(), data.c_str() + data.length(), &jsonRoot, &errs);

    //deng;解析出错
    if (!ok || errs.size() != 0)
    {
        // LOGW("invalid json: %s, client: %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        LOG_DEBUG("BussinessLogic::registerUser: 注册内容 解析出错，""invalid json =  %s, client = %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        delete reader;
        return;
    }
    delete reader;

    //deng; 必须有 用户名，昵称，密码。
    if (!jsonRoot["username"].isString() || !jsonRoot["nickname"].isString() || !jsonRoot["password"].isString())
    {
        LOG_DEBUG("BussinessLogic::registerUser: 注册信息不正确，""invalid json =  %s, client = %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        // LOGW("invalid json: %s, client: %s", data.c_str(), conn->peerAddress().toIpPort().c_str());        
        return;
    }

    User u;
    u.username = jsonRoot["username"].asString();
    u.nickname = jsonRoot["nickname"].asString();
    u.password = jsonRoot["password"].asString();

    //std::string retData;
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


    //conn->Send(msg_type_register, m_seq, retData);

    //LOGI << "Response to client: cmd=msg_type_register" << ", userid=" << u.userid << ", data=" << retData;
}