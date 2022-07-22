#include "EventLoopThread.h"
#include <functional>
#include "EventLoop.h"

using namespace net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
    const std::string& name/* = ""*/)
    : m_loop(NULL),
    m_exiting(false),
    m_callback(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if (m_loop != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
    {
        // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        m_loop->quit();
        m_thread->join();
    }
}

//dneg; 创建一个 std::thread，  在线程里创建一个 EventLoop ， 并调用 EventLoop:: loop(), 最后返回创建的 EventLoop 指针。
EventLoop* EventLoopThread::startLoop()
{
    //assert(!thread_.started());
    //thread_.start();

    m_thread.reset(new std::thread(std::bind(&EventLoopThread::threadFunc, this)));

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_loop == NULL)
        {
            m_cond.wait(lock);
        }
    }

    return m_loop;
}

//deng; 先退出loop， 再回收线程
void EventLoopThread::stopLoop()
{
    //deng: 这里用 while 会不会好一点， 如果退出loop需要很久怎么办？？
    //deng: 不会有任何问题，应为 m_thread->join() 会阻塞等待 线程退出即 线程回调函数退出。
    if (m_loop != NULL)
        m_loop->quit();

    //dneg: 感觉可能会有问题，当线程没有被创建的时候。
    m_thread->join();
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        //一个一个的线程创建
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify_all();  //deng； 放在大括号外面是否更好。
    }

    loop.loop();
    //assert(exiting_);
    m_loop = NULL;
}
