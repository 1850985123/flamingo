#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace net
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        EventLoopThreadPool();
        ~EventLoopThreadPool();

        void init(EventLoop* baseLoop, int numThreads);
        void start(const ThreadInitCallback& cb = ThreadInitCallback());

        void stop();

        /// round-robin
        EventLoop* getNextLoop();

        /// with the same hash code, it will always return the same EventLoop
        EventLoop* getLoopForHash(size_t hashCode);

        std::vector<EventLoop*> getAllLoops();

        bool started() const
        {
            return m_started;
        }

        const std::string& name() const
        {
            return m_name;
        }

        const std::string info() const;

    private:

        EventLoop* m_baseLoop;  //deng; 仅用于判断调用本线程池的线程是不是创建线程池的线程 ，运行 EventLoopThreadPool 的线程的 EventLoop
        std::string                                     m_name;
        bool                                            m_started;   //degn: 线程池是否启动
        int                                             m_numThreads;//deng: 线程数量
        int                                             m_next; // deng: 下一个loop在 vector中的索引

        //deng; 下面两个是一一对应的，直接用一个map保存是否更好 ?
        std::vector<std::unique_ptr<EventLoopThread> >  m_threads;//线程集合
        std::vector<EventLoop*>                         m_loops;// 每个线程对应的loop集合
    };

}
