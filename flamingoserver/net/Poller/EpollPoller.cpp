#include "EpollPoller.h"

#ifndef WIN32
#include <string.h>
#include "../../base/Platform.h"
#include "../../deng.h"




using namespace net;

namespace
{
    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    :m_epollfd(::epoll_create1(EPOLL_CLOEXEC)),
    m_events(kInitEventListSize),
    m_ownerLoop(loop)
{
    if (m_epollfd < 0)
    {
        LOG_ERROR("EPollPoller::EPollPoller , epoll_create1 函数创建 epollfd失败");
        LOGF("EPollPoller::EPollPoller");
    }
}

EPollPoller::~EPollPoller()
{
    ::close(m_epollfd);
}

bool EPollPoller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}

void EPollPoller::assertInLoopThread() const
{
    m_ownerLoop->assertInLoopThread();
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(m_epollfd,
        &*m_events.begin(),
        static_cast<int>(m_events.size()),
        timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //LOG_TRACE << " nothing happended";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll 中 epoll_wait()调用出错");
            // LOGSYSE("EPollPoller::poll()");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
        int fd = channel->fd();
        ChannelMap::const_iterator it = m_channels.find(fd);
        if (it == m_channels.end() || it->second != channel)
            return;
        channel->set_revents(m_events[i].events);
        activeChannels->push_back(channel);
    }
}

bool EPollPoller::updateChannel(Channel* channel)
{
    assertInLoopThread();
    LOGD("fd = %d  events = %d", channel->fd(), channel->events());
    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            //deng: 这里用 ！= 不知道是不是bug? 看完代码再说
            if (m_channels.find(fd) != m_channels.end())
            {
                LOGE("fd = %d  must not exist in channels_", fd);

                LOG_ERROR("EPollPoller::updateChannel,当前要新注册的 fd = %d 在 m_channels 里已经有了", fd);
                return false;
            }


            m_channels[fd] = channel;
        }
        else // index == kDeleted
        {
            if (m_channels.find(fd) == m_channels.end())
            {
                LOG_ERROR("EPollPoller::updateChannel,之前注册过的 fd = %d 在 m_channels 里没有了", fd);
                // LOGE("fd = %d  must exist in channels_", fd);
                return false;
            }

            //assert(channels_[fd] == channel);
            if (m_channels[fd] != channel)
            {
                LOG_ERROR("EPollPoller::updateChannel,之前注册过的 fd = %d 在 m_channels 里位置不匹配", fd);
                // LOGE("current channel is not matched current fd, fd = %d", fd);
                return false;
            }
        }
        channel->set_index(kAdded);

        return update(XEPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if (m_channels.find(fd) == m_channels.end() || m_channels[fd] != channel || index != kAdded)
        {
            LOGE("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
            return false;
        }

        if (channel->isNoneEvent())
        {
            //dneg；这里把监听的channel拿掉了， channel 还在 m_channels 里感觉好奇怪啊！！可以直接拿掉，应该是可以简化代码的
            if (update(XEPOLL_CTL_DEL, channel))
            {
                //deng: 有必要用这个状态吗？channel 要么就注册了事件要么就没有注册事件。好像得用因为如果之前有监听的事件，现在要修改的话。
                //deng: 这里直接把 channel 从channels里拿掉不就好了
                channel->set_index(kDeleted);
                return true;
            }
            return false;
        }
        else
        {
            return update(XEPOLL_CTL_MOD, channel);
        }
    }
}


//degn: 把 要移除的channel 从m_channels 中拿下来， 并去掉epoll中的事件监听。
void EPollPoller::removeChannel(Channel* channel)
{
    assertInLoopThread();
    int fd = channel->fd();

    if (m_channels.find(fd) == m_channels.end() || m_channels[fd] != channel || !channel->isNoneEvent())
        return;

    int index = channel->index();
    if (index != kAdded && index != kDeleted)
        return;

    size_t n = m_channels.erase(fd);
    if (n != 1)
        return;

    if (index == kAdded)
    {
        update(XEPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

bool EPollPoller:: update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(m_epollfd, operation, fd, &event) < 0)
    {
        // if (operation == XEPOLL_CTL_DEL)
        // {
        //     LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, m_epollfd, errno, strerror(errno));
        // }
        // else
        // {
        //     LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, m_epollfd, errno, strerror(errno));
        // }
        LOG_ERROR("EPollPoller:: update, epoll_ctl()函数出错， op=%d fd=%d, epollfd=%d, errno=%d, errorInfo = %s", operation, fd, m_epollfd, errno, strerror(errno));


        return false;
    }

    return true;
}

#endif
