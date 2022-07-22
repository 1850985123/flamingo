/***************************************************************
 * 版权所有 :
 * 文件名   : BusinessSession.h
 * 作者     : zhangyl
 * 版本     : 1.0.0.0
 * 创建日期 : 2019.03.29
 * 描述     :
 ***************************************************************/
#include "BusinessSession.h"
#include "../../UserManager.h"
#include "../websocketsrc/MyWebSocketServer.h"
#include "../../MsgCacheManager.h"

std::string BusinessSession::m_strWelcomeMsg = "Welcome to WebSocket Server, contact: balloonwj@qq.com";

BusinessSession::BusinessSession(std::shared_ptr<TcpConnection> &conn, int sessionid) : MyWebSocketSession(conn, sessionid)
{
}
void BusinessSession::onConnect()
{
    //发送欢迎语
    sendWelcomeMsg();
}

void BusinessSession::sendWelcomeMsg()
{
    send(BusinessSession::m_strWelcomeMsg);
}

bool BusinessSession::onMessage(const std::string &strClientMsg)
{
    // TODO: 收到的消息在这里处理，这里仅做一个消息回显

    LOG_DEBUG_WS("接收到websocket消息: %s, client: %s", strClientMsg.c_str(), getConnectionPtr()->peerAddress().toIpPort().c_str());

    MsgInfo msgInfo;
    if (!msgInfo.parseHttpMsg(strClientMsg))
        return false;

    Json::Value jsonRoot = msgInfo.getMsgJosnValue();

    int32_t cmd = msgInfo.getMsgCmd();
    std::shared_ptr<TcpConnection> conn = getConnectionPtr();
    switch (cmd)
    {
        //获取好友列表
    case msg_type_getofriendlist:
        if (!jsonRoot[0]["username"].isString())
        {
            LOG_DEBUG_WS("用户名格式错误");
            return false;
        }
        if (jsonRoot[0]["username"].asString() != m_userinfo.username)
        {
            LOG_DEBUG_WS("用户名和当前登录的用户名不一致，username = %s, login username = #%s",
                         jsonRoot[0]["username"].asString().c_str(), m_userinfo.username.c_str());
            return false;
        }

        //把好友信息发送给客户端 //把离线缓存的信息发送给客户端
        onGetFriendListResponse(conn);
        break;
    //聊天消息
    case msg_type_chat:
    {
        if (!jsonRoot[0]["targetid"].isInt())
        {
            LOG_DEBUG_WS("好友的 targetid 类型错误");
            return false;
        }

        onChatResponse(msgInfo, getConnectionPtr());
    }
    break;
        //查找用户
    case msg_type_finduser:
        onFindUserResponse(msgInfo, conn);
        break;
        // deng:  添加、删除等好友或群操作
    case msg_type_operatefriend:
        onOperateFriendResponse(msgInfo, conn);
        break;

    default:
        break;
    }
    // out = "{message:deng:"+out+"}";
    //  LOG_DEBUG_WS("进入到dneg");
    // out = "{message:deng:}";
    // LOG_DEBUG_WS("输出out的内容 = %s", out.c_str());

    // send(strClientMsg);

    return true;
}

void BusinessSession::onGetFriendListResponse(const std::shared_ptr<TcpConnection> &conn)
{
    std::string friendlist;
    makeUpFriendListInfo(friendlist, conn);
    std::ostringstream os;
    os << "{\"code\": 0, \"msg\": \"ok\", \"userinfo\":" << friendlist << "}";
    MsgInfo msgInfo;
    msgInfo.makeMsg(msg_type_getofriendlist, m_seq, os.str());
    send(msgInfo.getMsg());

    // LOG_DEBUG_WS("Response to client: userid: %d, cmd=msg_type_getofriendlist, data: %s", m_userinfo.userid, os.str().c_str());

    //推送离线通知消息
    std::list<NotifyMsgCache> listNotifyCache;
    Singleton<MsgCacheManager>::Instance().getNotifyMsgCache(m_userinfo.userid, listNotifyCache);
    for (const auto &iter : listNotifyCache)
    {
        if (!iter.notifymsgHttp.empty()) //因为其他客户端暂时没有添加http格式消息保存
            send(iter.notifymsgHttp);
    }

    //推送离线聊天消息
    std::list<ChatMsgCache> listChatCache;
    Singleton<MsgCacheManager>::Instance().getChatMsgCache(m_userinfo.userid, listChatCache);
    for (const auto &iter : listChatCache)
    {
        if (!iter.chatmsgHttp.empty()) //
            send(iter.chatmsgHttp);
    }

    //给其他用户推送上线消息
    // MyWebSocketServer &imserver = Singleton<MyWebSocketServer>::Instance();
    // std::list<User> friends;
    // Singleton<UserManager>::Instance().getFriendInfoByUserId(m_userinfo.userid, friends);
    // for (const auto &iter : friends)
    // {
    //     //因为存在一个用户id，多个终端，所以，同一个userid可能对应多个session
    //     std::list<std::shared_ptr<MyWebSocketSession>> sessions;
    //     imserver.getSessionsByUserId(sessions, iter.userid);
    //     for (auto &iter2 : sessions)
    //     {
    //         if (iter2)
    //         {
    //             iter2->sendUserStatusChangeMsg(m_userinfo.userid, 1, m_userinfo.status);

    //             LOGI("sendUserStatusChangeMsg to user(userid: %d): user go online, online userid: %d, status: %d", iter2->getUserId(), m_userinfo.userid, m_userinfo.status);
    //         }
    //     }
    // }
}

void BusinessSession::makeUpFriendListInfo(std::string &friendinfo, const std::shared_ptr<TcpConnection> &conn)
{
    std::string teaminfo;
    UserManager &userManager = Singleton<UserManager>::Instance();
    // ChatServer &imserver = Singleton<ChatServer>::Instance();
    userManager.getTeamInfoByUserId(m_userinfo.userid, teaminfo);

    /*
    [
    {
    "teamindex": 0,
    "teamname": "我的好友",
    "members": [
    {
    "userid": 1,

    },
    {
    "userid": 2,
    "markname": "张xx"
    }
    ]
    }
    ]
    */

    string markname = "";
    if (teaminfo.empty())
    {
        teaminfo = "[{\"teamname\": \"";
        teaminfo += DEFAULT_TEAMNAME;
        teaminfo += "\", \"members\": []}]";
    }

    Json::Value emptyArrayValue(Json::arrayValue);

    Json::CharReaderBuilder b;
    Json::CharReader *reader(b.newCharReader());
    Json::Value jsonRoot;
    JSONCPP_STRING errs;
    bool ok = reader->parse(teaminfo.c_str(), teaminfo.c_str() + teaminfo.length(), &jsonRoot, &errs);
    if (!ok || errs.size() != 0)
    {
        LOGE("parse teaminfo json failed, userid: %d, teaminfo: %s, client: %s", m_userinfo.userid, teaminfo.c_str(), conn->peerAddress().toIpPort().c_str());
        delete reader;
        return;
    }
    delete reader;

    if (!jsonRoot.isArray())
    {
        LOGE("parse teaminfo json failed, userid: %d, teaminfo: %s, client: %s", m_userinfo.userid, teaminfo.c_str(), conn->peerAddress().toIpPort().c_str());
        return;
    }

    // 解析分组信息，添加好友其他信息
    uint32_t teamCount = jsonRoot.size();
    int32_t userid = 0;

    // std::list<User> friends;
    User currentUserInfo;
    userManager.getUserInfoByUserId(m_userinfo.userid, currentUserInfo);
    User u;
    for (auto &friendinfo : currentUserInfo.friends)
    {
        for (uint32_t i = 0; i < teamCount; ++i)
        {
            // deng；确保成员类型是数组
            if (jsonRoot[i]["members"].isNull() || !(jsonRoot[i]["members"]).isArray())
            {
                jsonRoot[i]["members"] = emptyArrayValue;
            }
            // deng: 当前好友不在这个分组
            if (jsonRoot[i]["teamname"].isNull() || jsonRoot[i]["teamname"].asString() != friendinfo.teamname)
                continue;

            // deng: 当前已经有几个好友在分组里了
            uint32_t memberCount = jsonRoot[i]["members"].size();

            if (!userManager.getUserInfoByUserId(friendinfo.friendid, u))
                continue;

            if (!userManager.getFriendMarknameByUserId(m_userinfo.userid, friendinfo.friendid, markname))
                continue;

            // deng: 成员数量不应该还有一个循环吗？？
            jsonRoot[i]["members"][memberCount]["userid"] = u.userid;
            jsonRoot[i]["members"][memberCount]["username"] = u.username;
            jsonRoot[i]["members"][memberCount]["nickname"] = u.nickname;
            jsonRoot[i]["members"][memberCount]["markname"] = markname;
            jsonRoot[i]["members"][memberCount]["facetype"] = u.facetype;
            jsonRoot[i]["members"][memberCount]["customface"] = u.customface;
            jsonRoot[i]["members"][memberCount]["gender"] = u.gender;
            jsonRoot[i]["members"][memberCount]["birthday"] = u.birthday;
            jsonRoot[i]["members"][memberCount]["signature"] = u.signature;
            jsonRoot[i]["members"][memberCount]["address"] = u.address;
            jsonRoot[i]["members"][memberCount]["phonenumber"] = u.phonenumber;
            jsonRoot[i]["members"][memberCount]["mail"] = u.mail;

            jsonRoot[i]["members"][memberCount]["clienttype"] = 1;
            jsonRoot[i]["members"][memberCount]["status"] = 1;
            // jsonRoot[i]["members"][memberCount]["clienttype"] = imserver.getUserClientTypeByUserId(friendinfo.friendid);
            // jsonRoot[i]["members"][memberCount]["status"] = imserver.getUserStatusByUserId(friendinfo.friendid);
            ;
        } // end inner for-loop

    } // end outer for - loop

    // JsonRoot.toStyledString()返回的是格式化好的json，不实用
    // friendinfo = JsonRoot.toStyledString();
    // Json::FastWriter writer;
    // friendinfo = writer.write(JsonRoot);

    Json::StreamWriterBuilder streamWriterBuilder;
    streamWriterBuilder.settings_["indentation"] = "";
    friendinfo = Json::writeString(streamWriterBuilder, jsonRoot);
}

void BusinessSession::onChatResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn)
{

    std::string modifiedChatData;
    Json::Value jsonRoot = inputMsginfo.getMsgJosnValue();
    int32_t targetid = jsonRoot[0]["targetid"].asInt();
    // if (!modifyChatMsgLocalTimeToServerTime(data, modifiedChatData))
    // {
    //     LOGE("invalid chat json, chatjson: %s, senderid: %d, targetid: %d, chatmsg: %s, client: %s", data.c_str(), m_userinfo.userid, targetid, data.c_str(), conn->peerAddress().toIpPort().c_str());
    //     return;
    // }
    Json::FastWriter fwrite;
    std::string data = fwrite.write(jsonRoot[1]);
    modifiedChatData = data;

    /* 制作一般格式的消息 */
    std::string outbuf;
    BinaryStreamWriter writeStream(&outbuf);
    writeStream.WriteInt32(msg_type_chat);
    writeStream.WriteInt32(m_seq);
    writeStream.WriteString(modifiedChatData);
    //消息发送者
    writeStream.WriteInt32(m_userinfo.userid);
    //消息接受者
    writeStream.WriteInt32(targetid);
    writeStream.Flush();

    /* 制作 http格式的消息 */
    MsgInfo outMsgInfo;
    outMsgInfo.makeMsg(msg_type_chat, m_seq, modifiedChatData);
    // LOG_DEBUG_WS("消息1 = %s", outMsgInfo.getMsg().c_str());
    outMsgInfo.addInstruction("senderid", m_userinfo.userid);
    // LOG_DEBUG_WS("消息2 = %s", outMsgInfo.getMsg().c_str());
    outMsgInfo.addInstruction("targetid", targetid);
    // LOG_DEBUG_WS("消息3 = %s", outMsgInfo.getMsg().c_str());

    UserManager &userMgr = Singleton<UserManager>::Instance();
    //写入消息记录 //deng:一条消息需要知道：f_senderid， f_targetid， f_msgcontent， f_create_time（系统会自动添加）；
    //消息会保存到数据库，暂时还没有拿出来使用过，单纯保存一下
    if (!userMgr.saveChatMsgToDb(m_userinfo.userid, targetid, data))
    {
        LOG_ERROR_WS("Write chat msg to db error, senderid: %d, targetid: %d, chatmsg: %s, client: %s", m_userinfo.userid, targetid, data.c_str(), conn->peerAddress().toIpPort().c_str());
    }

    MyWebSocketServer &imserver = Singleton<MyWebSocketServer>::Instance();
    MsgCacheManager &msgCacheMgr = Singleton<MsgCacheManager>::Instance();
    //单聊消息
    if (targetid < GROUPID_BOUBDARY)
    {
        //先看目标用户是否在线
        std::list<std::shared_ptr<BusinessSession>> targetSessions;
        imserver.getSessionsByUserId(targetSessions, targetid);
        //目标用户不在线，缓存这个消息
        if (targetSessions.empty())
        {
            //添加缓存只需要知道，消息发送给谁，消息内容就ok; 数据保存在内存中。普通格式和http格式一起保存数据
            msgCacheMgr.addChatMsgCache(targetid, outbuf, outMsgInfo.getMsg());
        }
        else
        {
            for (auto &iter : targetSessions)
            {
                //如果要考虑多种客户端的话这里得做区分，这样做好吗？算了先只管web端
                if (iter)
                {

                    iter->send(outMsgInfo.getMsg());
                }
            }
        }
    }
    //群聊消息
    else
    {
        std::list<User> friends;
        userMgr.getFriendInfoByUserId(targetid, friends);
        std::string strUserInfo;
        bool useronline = false;
        for (const auto &iter : friends)
        {
            //排除群成员中的自己
            if (iter.userid == m_userinfo.userid)
                continue;

            //先看目标用户是否在线
            std::list<std::shared_ptr<BusinessSession>> targetSessions;
            imserver.getSessionsByUserId(targetSessions, iter.userid);
            //目标用户不在线，缓存这个消息
            if (targetSessions.empty())
            {
                msgCacheMgr.addChatMsgCache(iter.userid, outbuf, outMsgInfo.getMsg());
                continue;
            }
            else
            {
                for (auto &iter2 : targetSessions)
                {
                    if (iter2)
                        iter2->send(outMsgInfo.getMsg());
                }
            }
        }
    }
}

void BusinessSession::onFindUserResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn)
{
    //{ "type": 1, "username" : "zhangyl" }

    Json::Value jsonRoot = inputMsginfo.getMsgJosnValue();

    if (!jsonRoot[1]["type"].isInt() || !jsonRoot[1]["username"].isString())
    {
        LOG_DEBUG_WS("invalid json: %s, userid: %d, client: %s", inputMsginfo.getMsg().c_str(), m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
        return;
    }

    string retData;
    // TODO: 目前只支持查找单个用户
    string username = jsonRoot[1]["username"].asString();
    User cachedUser;
    if (!Singleton<UserManager>::Instance().getUserInfoByUsername(username, cachedUser))
        retData = "{ \"code\": 0, \"msg\": \"ok\", \"userinfo\": [] }";
    else
    {
        // TODO: 用户比较多的时候，应该使用动态string
        char szUserInfo[256] = {0};
        snprintf(szUserInfo, 256, "{ \"code\": 0, \"msg\": \"ok\", \"userinfo\": [{\"userid\": %d, \"username\": \"%s\", \"nickname\": \"%s\", \"facetype\":%d}] }", cachedUser.userid, cachedUser.username.c_str(), cachedUser.nickname.c_str(), cachedUser.facetype);
        retData = szUserInfo;
    }
    MsgInfo outputMsginfo;
    outputMsginfo.makeMsg(msg_type_finduser, m_seq, retData);
    send(outputMsginfo.getMsg());

    // LOG_DEBUG_WS("Response to client: userid: %d, cmd=msg_type_finduser, data: %s", m_userinfo.userid, retData.c_str());
}

// deng: （这些都是系统通知消息）操作有 加群、退群、删除好友、加好友、应答加好友、提示自己加好友成功、提示好友加好友成功
void BusinessSession::onOperateFriendResponse(const MsgInfo &inputMsginfo, const std::shared_ptr<TcpConnection> &conn)
{

    Json::Value jsonRoot = inputMsginfo.getMsgJosnValue();

    if (!jsonRoot[1]["type"].isInt() || !jsonRoot[1]["userid"].isInt())
    {
        LOG_DEBUG_WS("invalid json: %s, userid: %d, client: %s", inputMsginfo.getMsg().c_str(), m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
        return;
    }

    int type = jsonRoot[1]["type"].asInt();
    int32_t targetUserid = jsonRoot[1]["userid"].asInt();

    // deng: 这里用 userid可以区分是 群还是用户，用 username可以吗？如果username 不可以的话也就解释了为什么登录后都是用 userid在表里查询数据
    // deng: 客户端关于加好友和群的 userid来源，通过username 查询好友或群时服务器会返回，查询的信息，其中就有userid
    if (targetUserid >= GROUPID_BOUBDARY)
    {
        if (type == 4)
        {
            //退群
            // deleteFriend(conn, targetUserid);
            return;
        }

        if (Singleton<UserManager>::Instance().isFriend(m_userinfo.userid, targetUserid))
        {
            LOGE("In group already, unable to join in group, groupid: %d, , userid: %d, , client: %s", targetUserid, m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
            // TODO: 通知下客户端
            return;
        }

        //加群直接同意
        // onAddGroupResponse(targetUserid, conn);
        return;
    }

    char szData[256] = {0};
    //删除好友
    if (type == 4)
    {
        // deleteFriend(conn, targetUserid);
        return;
    }
    //发出加好友申请
    if (type == 1)
    {
        if (Singleton<UserManager>::Instance().isFriend(m_userinfo.userid, targetUserid))
        {
            LOGE("Friendship already, unable to add friend, friendid: %d, userid: %d, client: %s", targetUserid, m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
            // TODO: 通知下客户端
            return;
        }

        //{"userid": 9, "type": 1, }
        snprintf(szData, 256, "{\"userid\":%d, \"type\":2, \"username\": \"%s\"}", m_userinfo.userid, m_userinfo.username.c_str());
    }
    //应答加好友
    else if (type == 3)
    {
        if (!jsonRoot[1]["accept"].isInt())
        {
            LOG_DEBUG_WS("invalid json: %s, userid: %d, client: %s", inputMsginfo.getMsg().c_str(), m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
            return;
        }

        int accept = jsonRoot[1]["accept"].asInt();
        //接受加好友申请后，建立好友关系
        if (accept == 1)
        {
            if (!Singleton<UserManager>::Instance().makeFriendRelationshipInDB(targetUserid, m_userinfo.userid))
            {
                LOG_DEBUG_WS("make relationship error: %s, userid: %d, client:  %s", inputMsginfo.getMsg().c_str(), m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
                return;
            }

            if (!Singleton<UserManager>::Instance().updateUserRelationshipInMemory(m_userinfo.userid, targetUserid, FRIEND_OPERATION_ADD))
            {
                LOG_DEBUG_WS("UpdateUserTeamInfo error: %s, , userid: %d, client: %s", inputMsginfo.getMsg().c_str(), m_userinfo.userid, conn->peerAddress().toIpPort().c_str());
                return;
            }
        }

        //{ "userid": 9, "type" : 3, "userid" : 9, "username" : "xxx", "accept" : 1 }
        snprintf(szData, 256, "{\"userid\": %d, \"type\": 3, \"username\": \"%s\", \"accept\": %d}", m_userinfo.userid, m_userinfo.username.c_str(), accept);

        //提示自己当前用户加好友成功,把好友的id和名字和接收指令发回去
        User targetUser;
        if (!Singleton<UserManager>::Instance().getUserInfoByUserId(targetUserid, targetUser))
        {
            LOG_DEBUG_WS("Get Userinfo by id error, targetuserid: %d, userid: %d, data: %s, client: %s", targetUserid, m_userinfo.userid, inputMsginfo.getMsg().c_str(), conn->peerAddress().toIpPort().c_str());
            return;
        }
        char szSelfData[256] = {0};
        snprintf(szSelfData, 256, "{\"userid\": %d, \"type\": 3, \"username\": \"%s\", \"accept\": %d}", targetUser.userid, targetUser.username.c_str(), accept);

        MsgInfo selfOutMsg;
        selfOutMsg.makeMsg(msg_type_operatefriend, m_seq, szSelfData);
        send(selfOutMsg.getMsg());
        // LOGI("Response to client: userid: %d, cmd=msg_type_addfriend, data: %s", m_userinfo.userid, szSelfData);
    }

    //提示对方加好友成功
    std::string outbuf;
    BinaryStreamWriter writeStream(&outbuf);
    writeStream.WriteInt32(msg_type_operatefriend);
    writeStream.WriteInt32(m_seq);
    writeStream.WriteCString(szData, strlen(szData));
    writeStream.Flush();

    MsgInfo outMsg;
    outMsg.makeMsg(msg_type_operatefriend, m_seq, szData);

    //先看目标用户是否在线
    std::list<std::shared_ptr<BusinessSession>> sessions;
    Singleton<MyWebSocketServer>::Instance().getSessionsByUserId(sessions, targetUserid);
    //目标用户不在线，缓存这个消息
    if (sessions.empty())
    {
        Singleton<MsgCacheManager>::Instance().addNotifyMsgCache(targetUserid, outbuf, outMsg.getMsg());
        LOG_DEBUG_WS("userid: %d, is not online, cache notify msg, msg: %s", targetUserid, outMsg.getMsg().c_str());
        return;
    }

    for (auto &iter : sessions)
    {
        iter->send(outMsg.getMsg());
    }

    // LOGI("Response to client: userid: %d, cmd=msg_type_addfriend, data: %s", targetUserid, data.c_str());
}