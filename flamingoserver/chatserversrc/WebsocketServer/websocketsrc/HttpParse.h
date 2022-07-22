#pragma once

#include "../../../deng.h"
#include "HttpMsgDeal.h"
#include <unordered_map>

using namespace net;
class MyWebSocketSession;
class HttpParse
{
public:
    HttpParse();
    ~HttpParse() = default;

public:
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum PARSE_RESULT
    {
        PACKAGE_NOT_ENOUGH = 0, //包还没接收完整
        PACKAGE_ERROR,          //包解析错误
        UPGRADE_TO_WEBSOCE,     // http升级到websocket
        PARSE_SUCCEED           // http解析成功
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST, //文件不可读
        FILE_REQUEST,      //请求文件
        CONTENT_REQUEST,   // demg add 2022_7_14: 请求内容,
        INTERNAL_ERROR,    //
        CLOSED_CONNECTION
    };
    struct http_decodeInfo
    {
        METHOD method;              //请求的方法
        std::string strURL;         //请求路径
        std::string strParams_GET;  // GET请求提交的额参数（网址后面的参数）
        std::string strParams_POST; // POST请求提交的参数

        int content_length; //提交内容长度
        bool keep_alive;    // deng: 是否和客户端保持长连接。

        std::string request_file_path; //客户端的url请求对应的服务器资源完整路径。
        std::string requestFileType;   //请求文件类型 add 2022-7-21
        struct stat request_file_stat; //请求资源文件的属性

        std::string request_content_string; //有时候不一定是请求文件也有可能是请求内容。
        std::string request_httpHeader_string;
    };

public:
    HttpParse::PARSE_RESULT parse_start(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer);

    string getRespondContent()
    {
        return m_http_decodeInfo.request_httpHeader_string + m_http_decodeInfo.request_content_string;
    }
    string getRespondHttpHeader() { return m_http_decodeInfo.request_httpHeader_string; };
    string getRespondHttpcontent() { return m_http_decodeInfo.request_content_string; };

private:
    bool parseHttpPath(const std::string &str);
    bool parseHttpHeader(const string &data);
    void dealWithHttpWrite(const std::shared_ptr<TcpConnection> &conn);
    HTTP_CODE do_request(const std::shared_ptr<TcpConnection> &conn);
    bool process_write(HTTP_CODE ret);

private:
    void add_status_line(int status, const string &title);
    void add_headers(int content_len);
    void add_content_length(int content_len);
    void add_content_type(const string &content_type = "text/html");
    void add_linger();
    void add_blank_line();
    void add_content(const std::string &content);

private:
    bool isUpgradeToWebsocket();
    void makeUpgradeResponse(const char *secWebSocketAccept, std::string &response);

public:
    bool getClientCompressed() { return m_bClientCompressed; };
    OnlineUserInfo getLoginUserInfo() { return m_userinfo; };
    std::string getFileType(const std::string &suffix) //输入文件后缀得到文件的类型
    {
        if (m_fileTypeMap.find(suffix) == m_fileTypeMap.end())
            return m_fileTypeMap["default"];
        else
            return m_fileTypeMap[suffix];
    }

private:
    std::map<std::string, std::string> m_mapHttpHeaders;

    http_decodeInfo m_http_decodeInfo; //这个是http协议解析的信息集合
    std::string m_root;                //请求资源的根文件目录
    //客户端是否要求压缩

    bool m_bClientCompressed; // 用于确定之后的ewbsocket通信是否需要压缩

    bool m_httpHeaderParseComplete;

    HttpMsgDeal m_HttpMsgDeal;
    OnlineUserInfo m_userinfo;

private:
    static std::unordered_map<std::string, std::string> m_fileTypeMap;
};
