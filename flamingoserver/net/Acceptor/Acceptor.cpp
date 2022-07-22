#include "Acceptor.h"

#include "../../base/Platform.h"
#include "../InetAddress.h"
#include "../../deng.h"

using namespace net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : m_loop(loop),
    m_acceptSocket(sockets::createNonblockingOrDie()/*deng: 创建一个非阻塞的 Socket */),
    m_acceptChannel(loop, m_acceptSocket.fd()),
    m_listenning(false)
{
#ifndef WIN32
    m_idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
#endif

    m_acceptSocket.setReuseAddr(true); //deng: 端口复用
    m_acceptSocket.setReusePort(reuseport);
    m_acceptSocket.bindAddress(listenAddr); //degn: 绑定 ip 和 端口

    //deng: 设置通道读事件的回调函数， 这里的读事件就是 客户端连接事件。
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
#ifndef WIN32
    ::close(m_idleFd);
#endif
}

// deng: 监听客户端的连接
void Acceptor::listen()
{
    m_loop->assertInLoopThread();
    m_listenning = true;
    m_acceptSocket.listen();   //deng: 监听端口

    //deng: 使能通道的读事件监听。最终是往 epoll_fd 上添加读事件监听。
    m_acceptChannel.enableReading();
}

//deng: 监听到客户端的连接事件，会触发这个回调函数进行 accept, 并调用更上层的 m_newConnectionCallback 回调函数
void Acceptor::handleRead()
{
    m_loop->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = m_acceptSocket.accept(&peerAddr);
    if (connfd >= 0)
    {
        string hostport = peerAddr.toIpPort();
        LOGD("Accepts of %s", hostport.c_str());
        
        //newConnectionCallback_实际指向TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
        if (m_newConnectionCallback)
        {
            m_newConnectionCallback(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        LOGSYSE("in Acceptor::handleRead");

#ifndef WIN32
        /*
        The special problem of accept()ing when you can't

        Many implementations of the POSIX accept function (for example, found in post-2004 Linux)
        have the peculiar behaviour of not removing a connection from the pending queue in all error cases.

        For example, larger servers often run out of file descriptors (because of resource limits),
        causing accept to fail with ENFILE but not rejecting the connection, leading to libev signalling
        readiness on the next iteration again (the connection still exists after all), and typically
        causing the program to loop at 100% CPU usage.

        Unfortunately, the set of errors that cause this issue differs between operating systems,
        there is usually little the app can do to remedy the situation, and no known thread-safe
        method of removing the connection to cope with overload is known (to me).

        One of the easiest ways to handle this situation is to just ignore it - when the program encounters
        an overload, it will just loop until the situation is over. While this is a form of busy waiting,
        no OS offers an event-based way to handle this situation, so it's the best one can do.

        A better way to handle the situation is to log any errors other than EAGAIN and EWOULDBLOCK,
        making sure not to flood the log with such messages, and continue as usual, which at least gives
        the user an idea of what could be wrong ("raise the ulimit!"). For extra points one could
        stop the ev_io watcher on the listening fd "for a while", which reduces CPU usage.

        If your program is single-threaded, then you could also keep a dummy file descriptor for overload
        situations (e.g. by opening /dev/null), and when you run into ENFILE or EMFILE, close it,
        run accept, close that fd, and create a new dummy fd. This will gracefully refuse clients under
        typical overload conditions.

        The last way to handle it is to simply log the error and exit, as is often done with malloc
        failures, but this results in an easy opportunity for a DoS attack.
        */
        if (errno == EMFILE)
        {
            ::close(m_idleFd);
            m_idleFd = ::accept(m_acceptSocket.fd(), NULL, NULL);
            ::close(m_idleFd);
            m_idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
#endif
    }
}
