---
title: 高级IO函数
tags:
    - Linux
    - 网络编程
categories:
    - 技术笔记
---

# 一、概述
讨论几个问题：
- 为避免函数调用不可预期的永久阻塞，可以对IO操作设置超时，设置超时有哪些方法？
- 最基本的读写函数read/write，及其各种变体（send, recv, recvmsg....）是什么样的关系？
- 如果仅仅想查看套接字接收缓冲区的数据而不读取，要怎么做？

# 二、为套接字设置超时
抛出各种具体方法：
- 信号中断：alarm函数 + SIGALRM信号（缺点:干扰程序正常alarm的使用，在多线程程序中使用信号很困难）
- select保安：利用select可以设置超时，让select代为阻塞等待在IO上。
- 套接字选项：SO_RCVTIMEO和SO_SNDTIMEO（不能用于connect设置超时）

# 三、读写函数的各种变体
基本read/write及其三种变体send/recv、readv/writev和recvmsg/sendmsg

简要对比：
- send/recv对比read/write多了一个flags参数，可以传给内核；
- readv/writev对比read/write，后者是单个缓冲区的读写，前者则可以支持在单个系统调用中实现多个缓冲区的读写，称为“分散读”和“集中写”。
- recvmsg/sendmsg是最通用的函数，它集中了前面集中的所有特性。但是这组只能在套接字描述符中使用。
- sendto/recvfrom一般只在UDP套接字中使用。

## send/recv
```
#include <sys/socket.h>
ssize_t recv(int sockfd, void* buff, size_t bytes, int flags);
ssize_t send(int sockfd, const void* buff, size_t bytes, int flags);
```

**flags**——值参数
- MSG_DONTROUTE: 发送操作无须路由，仅限发送操作。表示目的主机在某个直接连接的本地网络上。
- **MSG_DONTWAIT**：单次操作“非阻塞”。
- MSG_OOB:对于send,表示即将发送的数据是带外数据（对于TCP只有一个字节）；对于recv表示，即将读入的是带外数据而不是普通数据。
- **MSG_PEEK**:表示仅查看可读取的数据，不作读取。适用于recv和recvmsg。
- **MSG_WAITALL**:表示尚未读到请求数目的字节之前不让一个读操作返回。意外情况:a. 捕获一个信号；b.连接被终止；c.套接字发生错误，相应的读函数仍旧可能返回少于请求数目的数据。

## readv/writev
```
#include <sys/io.h>
ssize_t readv(int flags, const struct iovec *iov, int iovcnt);
ssize_t writev(int flags, const struct iovec *iov, int iovcnt);

struct iovec {
    void *iov_base; //缓冲区起始地址
    void *iov_len; //缓冲区长度
};
```

注：
- writev是原子调用，对于数据报协议，一次writev调用只会产生一个UDP数据报。
- writev的一个用途：将小包数据经过整合，使用writev发送，可以有效防止Nagle算法的触发。

## recvmsg/sendmsg
具体参见UNP14.6

# 四、获悉已排队的数据量
方法：
- 如果获悉已排队数据量的目的在于不想阻塞，那么可以使用非阻塞调用。
- 如果既想查看数据，有想保留数据在接收队列中以供本进程其他部分稍后读取，可以使用MSG_PEEK。通常需要结合非阻塞+PEEK，要么是非阻塞套接字+MSG_PEEK,或者阻塞+MSG_DONTWAIT+MSG_PEEK。对于TCP，先只“获悉”，再次“读取”，两次调用的返回数据长度可能是不同的；对于UDP，两次调用的返回数据是完全相同的。
- ioctl + FIONREAD, 通过第三个参数（值——结果）返回当前接收队列的字节数。