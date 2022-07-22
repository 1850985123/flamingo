#pragma once
/*
    这个文件保存一些通用文件的头文件
*/

/******************** 日志系列   ************************/
#include "./base/tinyWebserver/log/log.h" //搞不明白这个路径为什么不行
// #include "/root/deng/flamingo/flamingoserver/base/tinyWebserver/log/log.h"
#include "./base/AsyncLog/AsyncLog.h"

/******************** base 系列   ************************/
#include "./base/Platform.h"
#include "./base/Timestamp.h"
#include "./base/Singleton.h"

/******************** net 网络框架   ************************/
#include "./net/Channel/Channel.h"
#include "./net/Sockets/Sockets.h"

#include "./net/Acceptor/Acceptor.h"
#include "./net/ByteBuffer/ByteBuffer.h"
#include "./net/EventLoop/EventLoop.h"
#include "./net/EventLoop/EventLoopThread.h"
#include "./net/EventLoop/EventLoopThreadPool.h"
#include "./net/Poller/EpollPoller.h"
#include "./net/Poller/Poller.h"
#include "./net/Poller/SelectPoller.h"
#include "./net/TcpClient/TcpClient.h"
#include "./net/TcpConnection/TcpConnection.h"
#include "./net/TcpServer/TcpServer.h"

#include "./net/InetAddress.h"
#include "./net/ProtocolStream.h"

/********************* zlib1.2.11  库相关 *********************/

#include "./zlib1.2.11/ZlibUtil.h"

/********************* utils  库相关 *********************/
#include "./utils/StringUtil.h"
#include "./utils/UUIDGenerator.h"

/********************* jsoncpp1.9.0  库相关 *********************/
#include "./jsoncpp1.9.0/json.h"

/********************* 标椎库的头文件 相关 *********************/
#include <sstream>