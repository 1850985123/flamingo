/**
 *  聊天服务程序入口函数
 *  zhangyl 2017.03.09
 **/
#include <iostream>
#include <stdlib.h>

#include "../base/Platform.h"
#include "../base/Singleton.h"
#include "../base/ConfigFileReader/ConfigFileReader.h"

#include "../mysqlmgr/MysqlManager.h"

#ifndef WIN32
#include <string.h>
#include "../utils/DaemonRun.h"
#endif

#include "UserManager.h"
#include "ChatServer.h"
#include "MonitorServer/MonitorServer.h"
#include "HttpServer/HttpServer.h"

#include "../deng.h"
#include "./WebsocketServer/websocketsrc/MyWebSocketServer.h"
#include "WebsocketServer/websocketsrc/HttpParse.h"
#include "WebsocketServer/websocketsrc/httpMsg.h"
using namespace net;

#ifdef WIN32
//初始化Windows socket库
NetworkInitializer windowsNetworkInitializer;
#endif

EventLoop g_mainLoop;
int system_run(void);

#ifndef WIN32
void prog_exit(int signo)
{
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    Singleton<MonitorServer>::Instance().uninit();
    Singleton<HttpServer>::Instance().uninit();
    Singleton<ChatServer>::Instance().uninit();
    g_mainLoop.quit();

    CAsyncLog::uninit();
}
#endif

void test()
{
    MsgInfo msgInfo;
    std::ostringstream strStream;
    strStream << "[{\"cmd\":1000,\"seq\":0},"
              << "{\"username\" :\"13917043329\",\"password\":\"123\",\"clienttype\":1,\"status\":1}]";

    string jsonString = "{\"aaa\":\"111111\",\"bbb\":222222,\"ccc\":{\"type\":\"log\"},\"ddd\":{\"type1\":\"00001\",\"type2\":\"00002\"},\"eee\":[{\"name\":\"Thomson\",\"age\":25,\"gender\":\"男\"},{\"name\":\"Rose\",\"age\":23,\"gender\":\"女\"}],\"fff\":{\"xxx1\":\"33333\",\"xxx2\":\"44444\",\"xxx3\":\"null\",\"xxx4\":{\"userId\":\"dss78fds8fds76fds6sd78fds\"},\"xxx5\":{\"code\":\"55555\",\"msg\":{\"yyy1\":\"6666\",\"yyy2\":\"7777\",\"yyy3\":\"888\"}}}}\n";

    // LOG_DEBUG_WS("%s", strStream.str().c_str());
    // msgInfo.parseHttpMsg(jsonString);
    // msgInfo.parseHttpMsg(strStream.str());
    // while (1)
    //     ;
}

int main(int argc, char *argv[])
{

    std::cout << " *************  main run   *************" << std::endl;

#ifndef WIN32
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, prog_exit);
    // signal(SIGTERM, prog_exit);

    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            bdaemon = true;
            break;
        }
    }

    if (bdaemon)
        daemon_run();
#endif

#ifdef WIN32
    CConfigFileReader config("../etc/chatserver.conf");
#else
    CConfigFileReader config("etc/chatserver.conf");
#endif

    const char *close_log_char = config.getConfigName("close_log");
    const char *LOGWrite_char = config.getConfigName("LOGWrite");

    //日志，初始化日志，设置是同步写还是异步写日志。
    int close_log = atoi(close_log_char);
    int LOGWrite = atoi(LOGWrite_char);

    // std::cout << "close_log = " << close_log << " ,LOGWrite = " << atoi(LOGWrite)<<endl;

    //初始化日志
    if (1 == LOGWrite)                                                             // deng: 异步写入日志
        Log::get_instance()->init(LOG_FIRST_DIR_NAME, close_log, 800000 * 2, 800); //"./ServerLog"
    else
        Log::get_instance()->init(LOG_FIRST_DIR_NAME, close_log, 800000 * 2, 0);

    test();

    LOG_INFO("/*********** 系统启动  *******************/");
    if (LOGWrite)
    {
        LOG_INFO("日志系统 : 异步写日志");
    }
    else
    {
        LOG_INFO("日志系统 : 同步写日志");
    }
    system_run();
    LOG_INFO("/*********** 系统退出  *******************/");

    Log::get_instance()->uninit();

    std::cout << " *************  main exit   *************" << std::endl;
    return 0;
}

int system_run()
{
#ifdef WIN32
    CConfigFileReader config("../etc/chatserver.conf");
#else
    CConfigFileReader config("etc/chatserver.conf");
#endif

    const char *logbinarypackage = config.getConfigName("logbinarypackage");
    if (logbinarypackage != NULL)
    {
        int logbinarypackageint = atoi(logbinarypackage);
        if (logbinarypackageint != 0)
            Singleton<ChatServer>::Instance().enableLogPackageBinary(true);
        else
            Singleton<ChatServer>::Instance().enableLogPackageBinary(false);
    }

    std::string logFileFullPath;
#ifndef WIN32
    const char *logfilepath = config.getConfigName("logfiledir");
    if (logfilepath == NULL)
    {
        LOGF("logdir is not set in config file");
        return 1;
    }

    //如果log目录不存在则创建之
    DIR *dp = opendir(logfilepath);
    if (dp == NULL)
    {
        if (mkdir(logfilepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            LOGF("create base dir error, %s , errno: %d, %s", logfilepath, errno, strerror(errno));
            return 1;
        }
    }
    closedir(dp);

    logFileFullPath = logfilepath;
#endif

    const char *logfilename = config.getConfigName("logfilename");
    logFileFullPath += logfilename;

#ifdef _DEBUG
    CAsyncLog::init();
#else
    CAsyncLog::init(logFileFullPath.c_str());
#endif

    //初始化数据库配置
    const char *dbserver = config.getConfigName("dbserver");
    const char *dbuser = config.getConfigName("dbuser");
    const char *dbpassword = config.getConfigName("dbpassword");
    const char *dbname = config.getConfigName("dbname");

    LOG_INFO("数据库系统：地址 = %s, 用户名 =  %s, 密码 = %s, 库名 = %s", dbserver, dbuser, dbpassword, dbname);
    if (!Singleton<CMysqlManager>::Instance().init(dbserver, dbuser, dbpassword, dbname))
    {
        // LOGF("Init mysql failed, please check your database config..............");
        LOG_ERROR("Singleton<CMysqlManager>::Instance().init()  : error ");
    }

    if (!Singleton<UserManager>::Instance().init(dbserver, dbuser, dbpassword, dbname))
    {
        // LOGF("Init UserManager failed, please check your database config..............");
        LOG_ERROR("Singleton<UserManager>::Instance().init()  : error ");
    }

    const char *listenip = config.getConfigName("listenip");
    short listenport = (short)atol(config.getConfigName("listenport"));

    LOG_INFO("ChatServer : ip = %s, port = %d", listenip, listenport);
    Singleton<ChatServer>::Instance().init(listenip, listenport, &g_mainLoop);

    const char *monitorlistenip = config.getConfigName("monitorlistenip");
    short monitorlistenport = (short)atol(config.getConfigName("monitorlistenport"));
    const char *monitortoken = config.getConfigName("monitortoken");

    LOG_INFO("MonitorServer : ip = %s, port = %d, monitortoken = %s", monitorlistenip, monitorlistenport, monitortoken);
    Singleton<MonitorServer>::Instance().init(monitorlistenip, monitorlistenport, &g_mainLoop, monitortoken);

    const char *httplistenip = config.getConfigName("monitorlistenip");
    short httplistenport = (short)atol(config.getConfigName("httplistenport"));

    LOG_INFO("HttpServer : ip = %s, port = %d", httplistenip, httplistenport);
    Singleton<HttpServer>::Instance().init(httplistenip, httplistenport, &g_mainLoop);

    // LOGI("chatserver initialization completed, now you can use client to connect it.");

    // websocketServer初始化
    Singleton<MyWebSocketServer>::Instance().init("0.0.0.0", 9999, &g_mainLoop);
    LOG_INFO("MyWebSocketServer : ip = %s, port = %d", "0.0.0.0", 9999);

    LOG_INFO("所有服务器初始化完成\r\n");

    g_mainLoop.loop();

    // LOGI("exit chatserver.");
}