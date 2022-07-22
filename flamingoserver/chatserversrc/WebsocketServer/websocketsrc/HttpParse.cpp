

#include "HttpParse.h"
#include <fstream>
#include <sstream>
#include "WebSocketHandshake.h"
#include "MyWebSocketSession.h"

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

//保存常用的文件类型
std::unordered_map<std::string, std::string> initial_m_fileTypeMap();
std::unordered_map<std::string, std::string> HttpParse::m_fileTypeMap(initial_m_fileTypeMap());

std::unordered_map<std::string, std::string> initial_m_fileTypeMap()
{
    std::unordered_map<std::string, std::string> m_fileTypeMap;
    m_fileTypeMap[".html"] = "text/html";
    m_fileTypeMap[".avi"] = "video/x-msvideo";
    m_fileTypeMap[".bmp"] = "image/bmp";
    m_fileTypeMap[".c"] = "text/plain";
    m_fileTypeMap[".doc"] = "application/msword";
    m_fileTypeMap[".gif"] = "image/gif";
    m_fileTypeMap[".gz"] = "application/x-gzip";
    m_fileTypeMap[".htm"] = "text/html";
    m_fileTypeMap[".ico"] = "image/x-icon";
    m_fileTypeMap[".jpg"] = "image/jpeg";
    m_fileTypeMap[".png"] = "image/png";
    m_fileTypeMap[".txt"] = "text/plain";
    m_fileTypeMap[".mp3"] = "audio/mp3";
    m_fileTypeMap["default"] = "text/html";

    m_fileTypeMap[".css"] = "text/css";
    m_fileTypeMap[".js"] = "application/javascript";
    return m_fileTypeMap;
}

void HttpParse::add_status_line(int status, const string &title)
{
    stringstream ss;
    ss << "HTTP/1.1 " << status << " " << title << " \r\n";
    m_http_decodeInfo.request_httpHeader_string = ss.str(); // deng: 第一次必须用等号，不然可能夹杂着上次请求的头信息
}

void HttpParse::add_headers(int content_len)
{
    // add_response("Access-Control-Allow-Origin: *\r\n");
    // add_response("X-Powered-By: Express\r\n");
    // add_response("Keep-Alive: timeout=5\r\n");
    // add_response("Etag: W/\"35-9rCX6n6l7wIRrra80LrG1Jt5qpQ\"\r\n");
    // LOG_PRINTF("当前使用的方法 = %d", m_http_decodeInfo.method);
    // if(m_http_decodeInfo.method == POST)
    //     add_response("Content-Type: application/json; charset=utf-8\r\n");
    // // add_response("Content-Type: text/html; charset=utf-8\r\n");

    add_content_length(content_len);
    if (m_http_decodeInfo.requestFileType.empty())
    {
        m_http_decodeInfo.requestFileType = getFileType("default");
    }
    add_content_type(m_http_decodeInfo.requestFileType);
    add_linger();
    add_blank_line();
}
void HttpParse::add_content_length(int content_len)
{
    stringstream ss;
    ss << "Content-Length:" << content_len << "\r\n";
    m_http_decodeInfo.request_httpHeader_string += ss.str();
}
void HttpParse::add_content_type(const string &content_type)
{

    stringstream ss;
    ss << "Content-Type:" << content_type << "\r\n";
    m_http_decodeInfo.request_httpHeader_string += ss.str();
}
void HttpParse::add_linger()
{
    stringstream ss;
    ss << "Connection:" << ((m_http_decodeInfo.keep_alive == true) ? "keep-alive" : "close") << "\r\n";
    m_http_decodeInfo.request_httpHeader_string += ss.str();
}
void HttpParse::add_blank_line()
{
    m_http_decodeInfo.request_httpHeader_string += "\r\n";
}
void HttpParse::add_content(const std::string &content)
{
    m_http_decodeInfo.request_httpHeader_string += content;
}

HttpParse::HttpParse()
{
    // root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    m_root = std::string(server_path, strlen(server_path));
    m_root += "/chatserversrc/WebsocketServer/resources/static";
};

// deng: 得到请求方法，请求路径， 请求参数
bool HttpParse::parseHttpPath(const std::string &str)
{
    std::vector<std::string> vecTags;
    StringUtil::split(str, vecTags, " ");

    // deng: 请求方法，请求路径（+ 参数），请求协议版本
    if (vecTags.size() != 3)
        return false;

    // deng: strcasecmp 不区分大小写的比较, 目前只支持 GET 和 POST,
    if (strcasecmp(vecTags[0].c_str(), "GET") == 0)
    {
        LOG_DEBUG_WS("请求方法 = GET");
        m_http_decodeInfo.method = GET;
    }
    else if (strcasecmp(vecTags[0].c_str(), "POST") == 0)
    {
        LOG_DEBUG_WS("请求方法 = POST");
        m_http_decodeInfo.method = POST;
    }
    else
    {
        LOG_DEBUG_WS("请求方法不正确！！！");
        return false;
    }

    std::vector<std::string> vecPathAndParams;
    StringUtil::split(vecTags[1], vecPathAndParams, "?");
    //至少有一个路径参数
    if (vecPathAndParams.empty())
        return false;

    m_http_decodeInfo.strURL = vecPathAndParams[0];
    if (vecPathAndParams.size() >= 2)
        m_http_decodeInfo.strParams_GET = vecPathAndParams[1];

    LOG_DEBUG_WS("请求路径 = %s", m_http_decodeInfo.strURL.c_str());

    if (!m_http_decodeInfo.strParams_GET.empty())
        LOG_DEBUG_WS("GET请求参数 = %s, len = %d", m_http_decodeInfo.strParams_GET.c_str(), m_http_decodeInfo.strParams_GET.length());

    // WebSocket协议版本号必须1.1
    if (vecTags[2] != "HTTP/1.1")
        return false;

    return true;
}

void HttpParse::makeUpgradeResponse(const char *secWebSocketAccept, std::string &response)
{
    response = "HTTP/1.1 101 Switching Protocols\r\n"
               "Upgrade: websocket\r\n"
               "Sec-Websocket-Accept: ";
    response += secWebSocketAccept;
    response += "\r\nServer: BTCMEXWebsocketServer 1.0.0\r\n";
    response += "Connection: Upgrade\r\n"
                "Sec-WebSocket-Version: 13\r\n";
    if (m_bClientCompressed)
        response += "Sec-WebSocket-Extensions: permessage-deflate; client_no_context_takeover\r\n";

    response += "Date: ";

    char szNow[64];
    time_t now = time(NULL);
    tm time;
#ifdef _WIN32
    localtime_s(&time, &now);
#else
    localtime_r(&now, &time);
#endif
    strftime(szNow, sizeof(szNow), "%Y%m%d%H%M%S", &time);
    response += szNow;
    response += "\r\n\r\n";
}

// deng: 如果是要升级到 Websocket 协议，放回true。
bool HttpParse::isUpgradeToWebsocket()
{

    /* TODO：针对客户端的要求
        握手必须是一个有效的 HTTP 请求
        请求的方法必须为 GET,且 HTTP 版本必须是 1.1
        请求的 REQUEST-URI 必须符合文档规定的要求(详情查看 Page 13)
        请求必须包含 Host 头
        请求必须包含 Upgrade: websocket 头,值必须为 websocket
        请求必须包含 Connection: Upgrade 头,值必须为 Upgrade
        请求必须包含 Sec-WebSocket-Key 头
        请求必须包含 Sec-WebSocket-Version: 13 头,值必须为 13
        请求必须包含 Origin 头
        请求可能包含 Sec-WebSocket-Protocol 头,规定子协议
        请求可能包含 Sec-WebSocket-Extensions ,规定协议扩展
        请求可能包含其他字段,如 cookie 等
     */

    auto target = m_mapHttpHeaders.find("Connection");
    if (target == m_mapHttpHeaders.end() || target->second != "Upgrade")
        return false;

    target = m_mapHttpHeaders.find("Upgrade");
    if (target == m_mapHttpHeaders.end() || target->second != "websocket")
        return false;

    target = m_mapHttpHeaders.find("Host");
    if (target == m_mapHttpHeaders.end() || target->second.empty())
        return false;

    target = m_mapHttpHeaders.find("Origin");
    if (target == m_mapHttpHeaders.end() || target->second.empty())
        return false;

    // TODO: 后面改成不区分大小写的
    //  target = m_mapHttpHeaders.find("User-Agent");
    //  if (target != m_mapHttpHeaders.end())
    //  {
    //      m_strUserAgent = target->second;
    //  }

    target = m_mapHttpHeaders.find("Sec-WebSocket-Extensions");
    if (target != m_mapHttpHeaders.end())
    {
        std::vector<std::string> vecExtensions;
        StringUtil::split(target->second, vecExtensions, ";");

        for (const auto &iter : vecExtensions)
        {
            if (iter == "permessage-deflate")
            {
                m_bClientCompressed = true;
                break;
            }
        }
    }

    target = m_mapHttpHeaders.find("Sec-WebSocket-Key");
    if (target == m_mapHttpHeaders.end() || target->second.empty())
        return false;

    char secWebSocketAccept[29] = {};
    js::WebSocketHandshake::generate(target->second.c_str(), secWebSocketAccept);
    std::string response;
    makeUpgradeResponse(secWebSocketAccept, m_http_decodeInfo.request_httpHeader_string);
    return true;
}

bool HttpParse::parseHttpHeader(const string &data)
{
    std::vector<std::string> vecHttpHeaders;
    StringUtil::split(data, vecHttpHeaders, "\r\n");
    //至少有3行
    if (vecHttpHeaders.size() < 3)
        return false;

    std::vector<std::string> v;
    size_t vecLength = vecHttpHeaders.size();

    // deng:  得到请求方法，请求路径， 请求参数
    if (!parseHttpPath(vecHttpHeaders[0]))
        return false;

    m_mapHttpHeaders.clear(); //防止之前的数据干扰
    for (size_t i = 1; i < vecLength; ++i)
    {
        //解析头标志
        v.clear();
        StringUtil::cut(vecHttpHeaders[i], v, ":");
        if (v.size() < 2)
            return false;

        StringUtil::trim(v[1]);
        // deng: 存放所有http头的键值对
        m_mapHttpHeaders[v[0]] = v[1];
    }

    auto target = m_mapHttpHeaders.find("Content-Length");
    if (target != m_mapHttpHeaders.end())
    {
        m_http_decodeInfo.content_length = stoi(target->second);
    }
    else
    {
        m_http_decodeInfo.content_length = 0;
    }

    m_http_decodeInfo.keep_alive = false;
    target = m_mapHttpHeaders.find("Connection");
    if (target != m_mapHttpHeaders.end() && target->second == "keep-alive")
        m_http_decodeInfo.keep_alive = true;

    target = m_mapHttpHeaders.find("Proxy-Connection");
    if (target != m_mapHttpHeaders.end() && target->second == "keep-alive")
        m_http_decodeInfo.keep_alive = true;

    // for(auto it = m_mapHttpHeaders.begin(); it != m_mapHttpHeaders.end(); it++ )
    // {
    //     LOG_DEBUG_WS("%s : %s", it->first.c_str(), it->second.c_str());
    // }
    return true;
}

HttpParse::PARSE_RESULT HttpParse::parse_start(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer)
{
    string str(pBuffer->peek(), pBuffer->readableBytes());

    // LOG_DEBUG_WS("接收到的内容:\r\n%s", str.c_str());

    // http头还没有被解析过
    if (!m_httpHeaderParseComplete)
    {
        // deng：①确保http请求头接收完成
        const char *pos = pBuffer->findString("\r\n\r\n");
        bool foundEngTag = (pos != nullptr);

        if (!foundEngTag)
        {
            //包还没收完
            if (pBuffer->readableBytes() < 2048)
                return PACKAGE_NOT_ENOUGH;
            else
                return PACKAGE_ERROR;
        }

        // 4是\r\n\r\n的长度
        size_t length = pos - (pBuffer->peek()) + 4;
        std::string data(pBuffer->peek(), length);
        pBuffer->retrieve(length);

        // deng: ②解析http请求头，
        if (!parseHttpHeader(data))
        {
            return PACKAGE_ERROR;
        }
        m_httpHeaderParseComplete = 1;
    }

    // deng:③解析http的请求内容，（post请求才有内容）
    if (m_http_decodeInfo.content_length > pBuffer->readableBytes())
    {
        LOG_DEBUG_WS("content_length = %d, readableBytes = %d", m_http_decodeInfo.content_length, pBuffer->readableBytes());
        return PACKAGE_NOT_ENOUGH;
    }
    else
    {
        m_httpHeaderParseComplete = 0;
        if (m_http_decodeInfo.content_length > 0) // degn:有post提交的参数
        {

            m_http_decodeInfo.strParams_POST = std::string(pBuffer->peek(), m_http_decodeInfo.content_length);
            pBuffer->retrieve(m_http_decodeInfo.content_length);
        }
    }

    // dneg:④判断是否要升级Websocket，
    if (isUpgradeToWebsocket())
    {
        LOG_DEBUG_WS("HttpMsgDeal::LoginUserInfo_map.size() = %d", HttpMsgDeal::LoginUserInfo_map.size());
        if (HttpMsgDeal::LoginUserInfo_map.find(conn->peerAddress().toIp()) != HttpMsgDeal::LoginUserInfo_map.end())
        {
            /* 得到登录的信息了 */
            LoginUserInfo m_loginUserInfo = HttpMsgDeal::LoginUserInfo_map[conn->peerAddress().toIp()];
            /* 已经得到登录的信息了，可以把map里的删除了 */
            HttpMsgDeal::LoginUserInfo_map.erase(conn->peerAddress().toIp());

            m_userinfo = m_loginUserInfo.m_userinfo;

            // m_http_decodeInfo.request_content_string = loginUserInfo.m_userinfo.username;

            LOG_DEBUG_WS("找到登录websocket的用户：当前ip = %s, 登录用户的ip = %s", conn->peerAddress().toIpPort().c_str(), m_loginUserInfo.clientipAndPort.c_str());
        }

        return UPGRADE_TO_WEBSOCE;
    }
    else
    {
        // dneg:处理发送的内容
        dealWithHttpWrite(conn);
        return PARSE_SUCCEED;
    }

    // LOG_DEBUG_WS("websocket message: %s", currentData.c_str());
}

HttpParse::HTTP_CODE HttpParse::do_request(const std::shared_ptr<TcpConnection> &conn)
{
    std::string tempUrl = m_http_decodeInfo.strURL;

    // LOG_DEBUG_WS("客户端实际请求的资源 Url:%s", tempUrl.c_str());

    /********************         自定义的内容处理                  *********************/
    if (strcasecmp(tempUrl.c_str(), "/") == 0) //修改定位到用于websocketServer
    {
        tempUrl = "/html/login.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/login") == 0)
    {
        // LOG_DEBUG_WS("进入");
        if (m_http_decodeInfo.method == POST)
        {
            MsgInfo msgInfo;
            if (msgInfo.parseHttpMsg(m_http_decodeInfo.strParams_POST))
            {
                LOG_DEBUG_WS("HttpParse::do_request 请求登录消息 = %s", msgInfo.getMsg().c_str());
                if (m_HttpMsgDeal.msgDeal(conn, msgInfo))
                {
                    // LOG_DEBUG_WS("进入2");
                    m_http_decodeInfo.request_content_string = m_HttpMsgDeal.getSengMsg().getMsg();
                    // LOG_DEBUG_WS("用户名 = %s, sessionid = %d", m_myWebSocketSession->getUsername().c_str(), m_myWebSocketSession->getSessionId());
                }

                // m_http_decodeInfo.request_content_string = "deng";
                return CONTENT_REQUEST;
            }
        }

        // tempUrl = "/html/chat.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/register") == 0)
    {
        // LOG_DEBUG_WS("进入");
        if (m_http_decodeInfo.method == POST)
        {
            MsgInfo msgInfo;
            if (msgInfo.parseHttpMsg(m_http_decodeInfo.strParams_POST))
            {
                LOG_DEBUG_WS("HttpParse::do_request 请求注册消息 = %s", msgInfo.getMsg().c_str());
                if (m_HttpMsgDeal.msgDeal(conn, msgInfo))
                {
                    // LOG_DEBUG_WS("进入2");
                    m_http_decodeInfo.request_content_string = m_HttpMsgDeal.getSengMsg().getMsg();
                    // LOG_DEBUG_WS("用户名 = %s, sessionid = %d", m_myWebSocketSession->getUsername().c_str(), m_myWebSocketSession->getSessionId());
                }

                // m_http_decodeInfo.request_content_string = "deng";
                return CONTENT_REQUEST;
            }
        }

        // tempUrl = "/html/chat.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/login.html") == 0)
    {

        tempUrl = "/html/login.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/register.html") == 0)
    {

        tempUrl = "/html/register.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/chat.html") == 0)
    {
        tempUrl = "/html/chat.html";
    }
    else if (strcasecmp(tempUrl.c_str(), "/getUsername") == 0)
    {

        LOG_DEBUG_WS("HttpMsgDeal::LoginUserInfo_map.size() = %d", HttpMsgDeal::LoginUserInfo_map.size());
        // LOG_DEBUG_WS("找到登录的用户：当前ip = %s, 登录用户的ip = %s",
        //              conn->peerAddress().toIpPort().c_str(), HttpMsgDeal::LoginUserInfo_map.begin()->first.c_str());

        // LOG_DEBUG_WS("HttpMsgDeal::LoginUserInfo_map.size() = %d", HttpMsgDeal::LoginUserInfo_map.size());
        // LOG_DEBUG_WS("用户名 = %s, sessionid = %d", m_myWebSocketSession->getUsername().c_str(), m_myWebSocketSession->getSessionId());
        if (HttpMsgDeal::LoginUserInfo_map.find(conn->peerAddress().toIp()) != HttpMsgDeal::LoginUserInfo_map.end())
        {

            LoginUserInfo loginUserInfo = HttpMsgDeal::LoginUserInfo_map[conn->peerAddress().toIp()];
            m_http_decodeInfo.request_content_string = loginUserInfo.m_userinfo.username;

            LOG_DEBUG_WS("找到登录的用户：当前ip = %s, 登录用户的ip = %s", conn->peerAddress().toIpPort().c_str(), loginUserInfo.clientipAndPort.c_str());
        }

        return CONTENT_REQUEST;
    }
    /********************         自定义的内容处理                  *********************/
    m_http_decodeInfo.request_file_path = m_root + tempUrl;
    LOG_DEBUG_WS("文件路径 = %s", m_http_decodeInfo.request_file_path.c_str());

    /*****  得到文件类型  *****/
    int dot_pos = m_http_decodeInfo.request_file_path.find('.');

    if (dot_pos < 0)
        m_http_decodeInfo.requestFileType = getFileType("default");
    else
        m_http_decodeInfo.requestFileType = getFileType(m_http_decodeInfo.request_file_path.substr(dot_pos));

    if (stat(m_http_decodeInfo.request_file_path.c_str(), &m_http_decodeInfo.request_file_stat) < 0)
        return NO_RESOURCE;

    if (!(m_http_decodeInfo.request_file_stat.st_mode & S_IROTH)) // deng: 文件不可读
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(m_http_decodeInfo.request_file_stat.st_mode)) // deng： 是一个目录
        return BAD_REQUEST;

    std::ifstream m_ifstream;
    m_ifstream.open(m_http_decodeInfo.request_file_path.c_str());
    std::ostringstream temp;
    temp << m_ifstream.rdbuf();
    m_http_decodeInfo.request_content_string = temp.str();
    // LOG_DEBUG_WS(" HttpParse::do_request, m_http_decodeInfo.request_content_string.size() = %d", m_http_decodeInfo.request_content_string.size());

    m_ifstream.close();

    // LOG_DEBUG_WS("文件内容 = %s", m_http_decodeInfo.request_content_string.c_str());

    return FILE_REQUEST;
}

bool HttpParse::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        LOG_DEBUG_WS("http_conn::process_write : 回应客户端， INTERNAL_ERROR");

        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        add_content(error_500_form);
        break;
    }
    case BAD_REQUEST:
    {
        LOG_DEBUG_WS("http_conn::process_write : 回应客户端， BAD_REQUEST");

        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        add_content(error_404_form);
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        LOG_DEBUG_WS("http_conn::process_write : 回应客户端， FORBIDDEN_REQUEST");

        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        add_content(error_403_form);
        break;
    }
    case CONTENT_REQUEST:
    case FILE_REQUEST:
    {
        LOG_DEBUG_WS("http_conn::process_write : 回应客户端， FILE_REQUEST");

        add_status_line(200, ok_200_title);
        // LOG_DEBUG_WS("add_status_line = %s", m_http_decodeInfo.request_httpHeader_string.c_str());

        // LOG_DEBUG_WS(" HttpParse::process_write, m_http_decodeInfo.request_content_string.size() = %d", m_http_decodeInfo.request_content_string.size());
        if (m_http_decodeInfo.request_content_string.size() != 0)
        {

            add_headers(m_http_decodeInfo.request_content_string.size());

            // LOG_DEBUG_WS("add_headers = %s", m_http_decodeInfo.request_httpHeader_string.c_str());
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            add_content(ok_string);
        }
    }
    default:
        return false;
    }
    return true;
}

void HttpParse::dealWithHttpWrite(const std::shared_ptr<TcpConnection> &conn)
{
    HTTP_CODE ret = do_request(conn);
    // LOG_DEBUG_WS("ret = %d", ret);

    process_write(ret);
}
