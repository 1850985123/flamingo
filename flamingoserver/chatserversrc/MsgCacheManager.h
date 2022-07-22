/**
 *  ��Ϣ�����࣬ MsgCacheManager.h
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
    bool addNotifyMsgCache(int32_t userid, const std::string &cache, const std::string &cacheHttp); //�������http��ʽ������//deng add 2022-7-20
    void getNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache> &cached);

    bool addChatMsgCache(int32_t userid, const std::string &cache);
    bool addChatMsgCache(int32_t userid, const std::string &cache, const std::string &cacheHttp); //�������http��ʽ������//deng add 2022-7-20
    void getChatMsgCache(int32_t userid, std::list<ChatMsgCache> &cached);

private:
    std::list<NotifyMsgCache> m_listNotifyMsgCache; //֪ͨ����Ϣ���棬����Ӻ�����Ϣ
    std::mutex m_mtNotifyMsgCache;
    std::list<ChatMsgCache> m_listChatMsgCache; //������Ϣ����
    std::mutex m_mtChatMsgCache;
};
