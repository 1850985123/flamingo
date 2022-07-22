cmake_minimum_required(VERSION 2.6)

project (FLAMGINGO_SERVER)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -Wall -O0 -Wno-unused-variable -pthread")

include_directories(
	/usr/include/uuid/
)

link_directories(
    ${PROJECT_SOURCE_DIR}/lib
	/usr/lib64/mysql/
)

# 推荐使用 CentOS 7.0 及以上系统
# CentOS 7.0 及以上系统默认安装的是mariadb，安装方法
# yum install mariadb-server mariadb mariadb-devel
# CentOS 7.0 以下系统默认安装mysql，安装方法：
# yum install mysql-server mysql mysql-devel

find_library(MYSQL_LIB libmysqlclient.so /usr/lib64/mysql/)
#如果你的数据库是mariadb，你还需要安装mariadb-devel库；如果你的数据库是mysql，你需要安装mysql-devel，否则会出现如下错误提示。
IF (NOT MYSQL_LIB)
    MESSAGE(FATAL_ERROR "mysqlclient not found. If your mysql is mariadb, please install mariadb-devel otherwise install mysql-devel.")
ENDIF(NOT MYSQL_LIB)

set(net_srcs 
base/AsyncLog/AsyncLog.cpp
base/ConfigFileReader/ConfigFileReader.cpp
base/Platform.cpp
base/Timestamp.cpp
base/tinyWebserver/log/log.cpp

net/Acceptor/Acceptor.cpp
net/ByteBuffer/ByteBuffer.cpp
net/Channel/Channel.cpp
net/Connector.cpp

net/Poller/EpollPoller.cpp
net/Poller/Poller.cpp
net/Poller/PollPoller.cpp
net/Poller/SelectPoller.cpp

net/ProtocolStream.cpp
net/EventLoop/EventLoop.cpp
net/EventLoop/EventLoopThread.cpp
net/EventLoop/EventLoopThreadPool.cpp
net/InetAddress.cpp
net/Sockets/Sockets.cpp
net/TcpClient/TcpClient.cpp
net/TcpConnection/TcpConnection.cpp
net/TcpServer/TcpServer.cpp
net/Timer.cpp
net/TimerQueue.cpp
)

set(mysqlapi_srcs
mysqlapi/DatabaseMysql.cpp
mysqlapi/Field.cpp
mysqlapi/QueryResult.cpp
)

set(mysqlmgr_srcs
mysqlmgr/MysqlManager.cpp
mysqlmgr/MysqlThrd.cpp
mysqlmgr/MysqlThrdMgr.cpp
mysqlmgr/TaskList.cpp
)

set(json_srcs
jsoncpp1.9.0/json_reader.cpp
jsoncpp1.9.0/json_value.cpp
jsoncpp1.9.0/json_writer.cpp
)

set(zlib_srcs
zlib1.2.11/zutil.c
zlib1.2.11/uncompr.c
zlib1.2.11/trees.c
zlib1.2.11/inftrees.c
zlib1.2.11/inflate.c
zlib1.2.11/inffast.c
zlib1.2.11/infback.c
zlib1.2.11/gzwrite.c
zlib1.2.11/gzread.c
zlib1.2.11/gzlib.c
zlib1.2.11/gzclose.c
zlib1.2.11/deflate.c
zlib1.2.11/crc32.c
zlib1.2.11/compress.c
zlib1.2.11/adler32.c
zlib1.2.11/ZlibUtil.cpp
)

set(utils_srcs
utils/StringUtil.cpp
utils/URLEncodeUtil.cpp
utils/MD5.cpp
utils/DaemonRun.cpp
)

set(chatserver_srcs
chatserversrc/main.cpp
chatserversrc/ChatServer.cpp
chatserversrc/ChatSession.cpp
chatserversrc/UserManager.cpp
chatserversrc/MsgCacheManager.cpp
chatserversrc/TcpSession.cpp

chatserversrc/MonitorServer/MonitorSession.cpp
chatserversrc/MonitorServer/MonitorServer.cpp
chatserversrc/HttpServer/HttpSession.cpp
chatserversrc/HttpServer/HttpServer.cpp

chatserversrc/BussinessLogic.cpp)

set(fileserver_srcs
fileserversrc/main.cpp
fileserversrc/FileServer.cpp
fileserversrc/FileSession.cpp
fileserversrc/FileManager.cpp
fileserversrc/TcpSession.cpp)

set(imgserver_srcs
imgserversrc/main.cpp
fileserversrc/FileServer.cpp
fileserversrc/FileSession.cpp
fileserversrc/FileManager.cpp
fileserversrc/TcpSession.cpp)

set(websocketServer_srcs
chatserversrc/WebsocketServer/websocketsrc/MyWebSocketServer.cpp
chatserversrc/WebsocketServer/websocketsrc/MyWebSocketSession.cpp
chatserversrc/WebsocketServer/appsrc/BusinessSession.cpp
chatserversrc/WebsocketServer/websocketsrc/HttpParse.cpp
chatserversrc/WebsocketServer/websocketsrc/httpMsg.cpp
chatserversrc/WebsocketServer/websocketsrc/HttpMsgDeal.cpp
)



add_executable(chatserver ${net_srcs} ${json_srcs}  ${chatserver_srcs} ${mysqlapi_srcs} ${mysqlmgr_srcs} ${zlib_srcs} ${utils_srcs} ${websocketServer_srcs})
#光包含库目录是没用的，还必须使用TARGET_LINK_LIBRARIES链接该库
TARGET_LINK_LIBRARIES(chatserver mysqlclient)

add_executable(fileserver ${net_srcs}  ${fileserver_srcs} ${utils_srcs})
TARGET_LINK_LIBRARIES(fileserver)

add_executable(imgserver ${net_srcs}  ${imgserver_srcs} ${utils_srcs})
TARGET_LINK_LIBRARIES(imgserver)