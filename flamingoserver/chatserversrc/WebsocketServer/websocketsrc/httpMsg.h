#pragma once

// #include "../../../deng.h"
#include "../../Msg.h"
#include "../../../jsoncpp1.9.0/json.h"
#include <string>

struct chat_msg_header;
class MsgInfo
{
public:
    MsgInfo() = default;
    ~MsgInfo() = default;

public:
    bool parseHttpMsg(const std::string &data);
    // std::string toHttpMsgstd::string() const;
    // std::string toChatMsgCachestd::string() const;
    void addInstruction(std::string key, std::string value);
    void addInstruction(std::string key, int32_t value);

public:
    // inline std::string getMsgCentent() const { return m_chat_msg_centent; };
    inline Json::Value getMsgJosnValue() const { return m_jsonRoot; };
    inline int32_t getMsgSeq() const { return m_seq; };
    inline int32_t getMsgCmd() const { return m_cmd; };
    inline std::string getMsg() const { return m_chat_msg; };

public:
    // inline void setMsgCentent(std::string m_chat_msg_centent) { this->m_chat_msg_centent = m_chat_msg_centent; };
    // inline void setMsgSeq(int32_t m_seq) { this->m_seq = m_seq; };
    // inline void setMsgCmd(int32_t m_cmd) { this->m_cmd = m_cmd; };
    inline void makeMsg(std::string m_chat_msg)
    {
        this->m_chat_msg = m_chat_msg;
    };
    inline void makeMsg(int32_t m_cmd, int32_t m_seq)
    {
        makeMsg(m_cmd, m_seq, "");
    };
    inline void makeMsg(int32_t m_cmd, int32_t m_seq, std::string centent)
    {
        std::ostringstream strStream;
        strStream << "[{\"cmd\":" << m_cmd << ",\"seq\":" << m_seq << "}," << centent << "]";
        // m_chat_msg = strStream.str();
        this->parseHttpMsg(strStream.str());
    };
    //添加多余的命令信息
    // inline void addInstruction(int32_t m_cmd, int32_t m_seq, std::string centent)
    // {
    //     std::ostringstream strStream;
    //     strStream << "[{\"cmd\":" << m_cmd << ",\"seq\":" << m_seq << "}," << centent << "]";
    //     m_chat_msg = strStream.str();
    // };
    inline Json::Value &getJsonValueRef()
    {
        return m_jsonRoot;
    }

private:
    Json::Value m_jsonRoot;
    // chat_msg_header m_chat_msg_header;
    std::string m_chat_msg;

    int32_t m_cmd;
    int32_t m_seq;
};
