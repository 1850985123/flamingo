#include "httpMsg.h"
#include "../../../deng.h"

//包最大字节数限制为10M
#define MAX_PACKAGE_SIZE 10 * 1024 * 1024

bool MsgInfo::parseHttpMsg(const std::string &data)
{
    Json::CharReaderBuilder b;
    Json::CharReader *reader(b.newCharReader());
    JSONCPP_STRING errs;

    bool ok = reader->parse(data.c_str(), data.c_str() + data.length(), &m_jsonRoot, &errs);
    if (!ok || errs.size() != 0)
    {
        LOG_DEBUG_WS("error: %s", errs.c_str());
        LOG_DEBUG_WS("MsgInfo::parseHttpMsg ERROR, invalid json;");
        delete reader;
        return false;
    }
    delete reader;

    if (!m_jsonRoot.isArray())
    {
        LOG_DEBUG_WS("消息不是json 数组格式，message = %s", data.c_str());
        return false;
    }
    /* 解析消息头*/
    // if (!m_jsonRoot[0]["compressflag"].isBool() ||
    //     !m_jsonRoot[0]["originsize"].isInt() ||
    //     !m_jsonRoot[0]["compresssize"].isInt())
    //     return false;
    // m_chat_msg_header.compressflag = m_jsonRoot[0]["compressflag"].asBool();
    // m_chat_msg_header.originsize = m_jsonRoot[0]["originsize"].asInt();
    // m_chat_msg_header.compresssize = m_jsonRoot[0]["compresssize"].asInt();
    // LOG_DEBUG_WS("compressflag = %d, originsize = %d, compresssize = %d",
    //              m_chat_msg_header.compressflag, m_chat_msg_header.originsize, m_chat_msg_header.compresssize);

    /* 判断消息的 cmd 和 seq 是否正确 */
    if (!m_jsonRoot[0]["cmd"].isInt() || !m_jsonRoot[0]["seq"].isInt())
    {
        LOG_DEBUG_WS("cmd 或者 seq 格式不对, message = %s", data.c_str());
        return false;
    }
    m_cmd = m_jsonRoot[0]["cmd"].asInt();
    m_seq = m_jsonRoot[0]["seq"].asInt();

    m_chat_msg = data;

    /* 解析消息内容 */
    // if (m_jsonRoot.size() == 3)
    // {
    // }

    /* 解析消息内容 */
    // std::vector<std::string> v;
    // StringUtil::split(data, v, "\"chat_msg_content\":");
    // if (v.size() != 2) //
    //     return false;
    // m_chat_msg_centent = v[1];
    // StringUtil::replace(m_chat_msg_centent, "}}", "}");
    // LOG_DEBUG_WS("m_chat_msg_centent = %s", m_chat_msg_centent.c_str());
    // LOG_DEBUG_WS("%s", m_jsonRoot.toStyledString().c_str());

    // /* 判断消息大小是否正确一致 */
    // if (m_chat_msg_header.compressflag == true)
    // {
    //     //包头有错误，立即关闭连接
    //     if (m_chat_msg_header.compresssize <= 0 || m_chat_msg_header.compresssize > MAX_PACKAGE_SIZE ||
    //         m_chat_msg_header.originsize <= 0 || m_chat_msg_header.originsize > MAX_PACKAGE_SIZE)
    //     {
    //         LOG_DEBUG_WS("compresssize: 包大小出错");
    //         return false;
    //     }

    //     if (m_chat_msg_header.compresssize != m_chat_msg_centent.size())
    //     {
    //         LOG_DEBUG_WS("compresssize: 包的实际大小和包头大小不一致");
    //         return false;
    //     }
    // }
    // else
    // {
    //     /* 判断消息大小是否正确 */
    //     if (m_chat_msg_header.originsize <= 0 || m_chat_msg_header.originsize > MAX_PACKAGE_SIZE)
    //     {
    //         LOG_DEBUG_WS("originsize: 包大小出错");
    //         return false;
    //     }

    //     if (m_chat_msg_header.originsize != m_chat_msg_centent.size())
    //     {
    //         LOG_DEBUG_WS("originsize: 包的实际大小和包头大小不一致");
    //         return false;
    //     }
    // }

    // Json::StreamWriterBuilder streamWriterBuilder;
    // //消除json中的\t和\n符号
    // // streamWriterBuilder.settings_["indentation"] = "";
    // string chatOutputJson = Json::writeString(streamWriterBuilder, m_jsonRoot[3]);

    // Json::FastWriter fwriter;
    // Json::StyledWriter swriter;
    // string chatOutputJson = fwriter.write(m_jsonRoot);
    // LOG_DEBUG_WS(" m_jsonRoot[3] = %s", chatOutputJson.c_str());

    // LOG_DEBUG_WS(" m_jsonRoot[0].size() = %d", m_jsonRoot[0].size());
    // LOG_DEBUG_WS(" m_jsonRoot[1].size() = %d", m_jsonRoot[1].size());
    // LOG_DEBUG_WS(" m_jsonRoot[2].size() = %d", m_jsonRoot[2].size());

    // chatOutputJson = swriter.write(m_jsonRoot);
    // LOG_DEBUG_WS(" m_jsonRoot[3] = %s", chatOutputJson.c_str());
    // LOG_DEBUG_WS("%s", m_jsonRoot.toStyledString().c_str());
    // m_jsonRoot[0].
    // LOG_DEBUG_WS("%s", str.c_str());
    // LOG_DEBUG_WS("%s", m_jsonRoot.toStyledString().c_str());
    // LOG_DEBUG_WS("m_jsonRoot[\"chat_msg_content\"][\"originsize\"] = %d", m_jsonRoot[0]["originsize"].asInt());

    // LOG_DEBUG_WS("m_jsonRoot[\"chat_msg_header\"] = %s", m_jsonRoot[0].toStyledString().c_str());
    // LOG_DEBUG_WS("m_jsonRoot[\"chat_msg_content\"] = %s", m_jsonRoot["chat_msg_content"].toStyledString().c_str());

    // LOG_DEBUG_WS("%s", m_jsonRoot[0].asCString());
    // LOG_DEBUG_WS("%s", m_jsonRoot["chat_msg_content"].asCString());
    return true;
};

// string MsgInfo::toHttpMsgString() const
// {
//     // std::ostringstream strStream;
//     // m_chat_msg_header.compressflag = false; // http协议暂时不压缩
//     // m_chat_msg_header.originsize = m_chat_msg_centent.size();
//     // m_chat_msg_header.compresssize = 0;

//     // strStream << "{\"chat_msg_header\":"
//     //           << "{\"compressflag\"" << m_chat_msg_header.compressflag
//     //           << ",\"originsize\":" << m_chat_msg_header.originsize
//     //           << ",\"compresssize\":" << m_chat_msg_header.compresssize
//     //           << "},"
//     //           << "\"cmd\":" << m_cmd << ",\"seq\":" << m_seq << ","
//     //           << "\"chat_msg_content\":" << m_chat_msg_centent;
//     // return strStream.str();
//     string str;
//     return str;
// }

// string MsgInfo::toChatMsgCacheString() const
// {
//     //  Json::StreamWriterBuilder streamWriterBuilder;
//     // //消除json中的\t和\n符号
//     // streamWriterBuilder.settings_["indentation"] = "";
//     // chatOutputJson = Json::writeString(streamWriterBuilder, jsonRoot);
//     string str;
//     return str;
// }

void MsgInfo::addInstruction(std::string key, std::string value)
{
    if (!m_jsonRoot.isArray())
    {
        LOG_DEBUG_WS("MsgInfo::addInstruction error");
        return;
    }
    m_jsonRoot[0][key] = value;
    Json::FastWriter fwriter;
    this->m_chat_msg = fwriter.write(m_jsonRoot);
}
void MsgInfo::addInstruction(std::string key, int32_t value)
{
    if (!m_jsonRoot.isArray())
    {
        LOG_DEBUG_WS("MsgInfo::addInstruction error");
        return;
    }
    m_jsonRoot[0][key] = value;
    Json::FastWriter fwriter;
    this->m_chat_msg = fwriter.write(m_jsonRoot);
}