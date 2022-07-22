/**
 *  消息缓存类， MsgCacheManager.h
 *  zhangyl 2017.03.16
 **/
#pragma once
#include <list>
#include <stdint.h>
#include <string>
#include <mutex>

struct NotifyMsgCache
{
    int32_t userid;
    std::string notifymsg;
    std::string notifymsgHttp; // deng add 2022-7-20
};

struct ChatMsgCache
{
    int32_t userid;
    std::string chatmsg;
    std::string chatmsgHttp; // deng add 2022-7-20
};

class MsgCacheManager final
{
public:
    MsgCacheManager();
    ~MsgCacheManager();

    MsgCacheManager(const MsgCacheManager &rhs) = delete;
    MsgCacheManager &operator=(const MsgCacheManager &rhs) = delete;

    bool addNotifyMsgCache(int32_t userid, const std::string &cache);
    bool addNotifyMsgCache(int32_t userid, const std::string &cache, const std::string &cacheHttp); //可以添加http格式的数据//deng add 2022-7-20
    void getNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache> &cached);

    bool addChatMsgCache(int32_t userid, const std::string &cache);
    bool addChatMsgCache(int32_t userid, const std::string &cache, const std::string &cacheHttp); //可以添加http格式的数据//deng add 2022-7-20
    void getChatMsgCache(int32_t userid, std::list<ChatMsgCache> &cached);

private:
    std::list<NotifyMsgCache> m_listNotifyMsgCache; //通知类消息缓存，比如加好友消息
    std::mutex m_mtNotifyMsgCache;
    std::list<ChatMsgCache> m_listChatMsgCache; //聊天消息缓存
    std::mutex m_mtChatMsgCache;
};
