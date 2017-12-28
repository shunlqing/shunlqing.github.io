---
title: linux文件IO
tags:
    - linux
categories:
    - 操作系统
---

# 文件描述符和句柄

句柄，从一般意义上讲，句柄作为内核与用户的交互。用户无法指定内核对象的本体，内核可以提供一个整数值，在内核创建内核对象的时候，返回给用户。用户在想调用该对象的时候，可以指定。

例如，文件在内核的活动对象是一个个file结构，对于用户来说，文件描述符代表这一个文件的标识。用户可以指定文件描述符，内核通过查找来找到文件管理结构file.

因此，文件描述符可以称为“文件句柄”。

# 内核文件表

每个进程管理一张打开文件表，用以管理当前进程所打开的文件，称为“进程打开文件表”，保存在task_struct->files字段，是一个files_struct结构体。

files_struct结构的功能是管理一个file结构**指针**数组，**文件描述符**就是用来索引该数组，从而找到真正的文件管理结构file.此外，files_struct的两个重要字段：
- close_on_exec_init: 表示当执行exec时要关闭的文件描述符位图
- open_fds_init: 保存打开的文件描述符位图

# 文件管理结构file
```
struct file {
    /*...*/
    const struct file_operations *f_op; 与该文件相关联的操作函数
    atomic_t f_count; 文件的引用计数(有多少进程打开该文件)
    unsigned int f_flags; 对应于open时指定的flag
    mode_t f_mode; 读写模式：open的mod_t mode参数
    off_t f_pos; 该文件在当前进程中的文件偏移量
    /*...*/
};
```

![](http://p1lv91x5b.bkt.clouddn.com/struct_file.png)

# 文件操作
## open操作
打开一个文件，内核都做了些什么呢？
- 在进程打开文件表中找到一个未使用的文件描述符
- 申请一个新的文件管理结构file,根据不同的打开标志和mode产生不同的行为和file结构。
- 绑定文件描述符和对应的文件管理结构file，把文件描述符返回给用户。

可见，open操作，内核主要消耗两种资源：文件描述符和文件管理结构file。

open参数
- pathname : 文件路径
- flags: 打开标志
- mode：文件的权限位，只在创建文件时需要，并受到umask的影响。

**每一次open操作,都将分配一个文件描述符和文件管理结构,即使打开的是同一个文件.**

## close操作
close用于关闭文件描述符。而文件描述符可以是普通文件，也可以是设备，还可以是socket.在关闭时，VFS会根据不同的文件类型，执行不同的操作。这个实现机制由file结构的fop字段绑定的具体操作函数实现。

**遗忘close造成的问题**
- 文件描述符没有释放
- 文件管理结构没有释放
- 对于普通进程，在进程结束时，内核自动回收；但是对于常驻进程，问题相当严重。

**如何查找文件资源泄露**——lsof命令

## 文件偏移
文件偏移量是打开文件中比较重要的属性.它代表当前进程对**当前打开文件**的读写位置.一般情况下,读写操作都是从当前的偏移位置开始读写, 并且在读写结束之后更新偏移量.

刚打开文件时, 文件偏移量为0;

进程fork之后,父子进程的对同一打开文件的文件偏移量是一样的吗?后面解答.

## 读文件_read
```
ssize_t read(int fd, void* buf, size_t count);
```
最一般意义,read尝试从fd中读取count个字节到用户定义缓冲区buf中,并返回实际成功读取的字节数.

**意外情况**:返回值为-1
- errno = EAGAIN, EWOULDBLOCK: fd为非阻塞且没有数据可返回时
- errno = EINTR: 信号中断

**部分读取**

不同类型的文件出现部分读取的情况是不同的,根据具体绑定的fops->read函数而定:
- 普通文件,到达文件末尾 EOF
- socket文件系统的UDP: UDP报文数据长度小于参数len时,返回实际的数据长度
- ...(根据不同的类型小心处理)


## 写文件_write
```
ssize_t write(int fd, const void*buf, size_t count);
```
write操作根据当前fd的文件偏移量, 写入count个字节数据.与read操作类似,同样会出现**部分写**的情况.

**追加写**--O_APPEND

当使用O_APPEND以追加的形式打开文件的时候,每次写操作都会先定位到文件的末尾,然后再执行写操作.每次写操作获取到的都是文件(inode)的最新末尾位置(对inode加锁保护, 以避免多进程情况下竞争).
 
## 文件描述符的复制
```
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
```
- dup:使用一个最小的未使用的文件描述符作为复制后的文件描述符
- dup2:使用用户指定的newfd来复制oldfd,如果newfd已经打开,就先关闭newfd,在复制oldfd(close+dup的原子性调用)
- dup3:支持O_CLOEXEC.表示新复制的fd在fork并执行exec之后,将关闭.

**复制的只是文件描述符**

新复制的文件描述符和旧的文件描述符指向的是同一个file文件管理结构,他们共享文件偏移量,文件读写模式,文件打开标志等等. file的创建仅仅在open操作的时候发生.

## 文件同步

## 文件截断

# 进程执行fork后，文件打开表和文件管理结构file

