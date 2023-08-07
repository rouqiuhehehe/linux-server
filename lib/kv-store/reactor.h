//
// Created by Yoshiki on 2023/8/7.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
#define LINUX_SERVER_LIB_KV_STORE_EPOLL_H_

#include <functional>
class Reactor
{
    using Callback = std::function <void (void)>;
private:
    Callback acceptCb;
    Callback recvCb;
    Callback sendCb;
};
#endif //LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
