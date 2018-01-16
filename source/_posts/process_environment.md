---
title: 进程环境
tags:
    - Linux
categories:
    - 操作系统
    - 技术笔记
---

讨论进程环境中几个点：
- 进程终止：exit和_exit的关系
- 环境变量的设置,putenv的陷阱
- 内存问题
- longjmp的原理与陷阱

# exit和_exit
当一个进程正常终止时，系统需要做一系列的处理。这里涉及到两个层面：C库和内核的处理：
- C库层做的工作：刷新IO缓冲区，执行所有注册的退出函数；
- 内核层做的工作：回收进程所持有的一些资源；

进程正常退出时，先执行exit函数(C库层面)，再在exit函数里面调用_exit陷入内核，执行内核退出进程的操作。但异常退出又是怎么样的呢？异常退出只会执行内核退出进程的操作。

正常退出：
- main使用return；return(n)相当于exit(n);
- 直接使用exit(n)

异常退出：
- 使用_exit(n),因为没有正常执行C库层面的清理工作，所以我们把它归纳到异常状况。但是有时候_exit是必要的，比如多进程的时候。
- 信号中断。当出现信号中断，信号中断很多的处理都是进程终止，此时执行的直接是内核退出进程的操作，不会返回到C库执行清理工作。

## exit
```
    void exit(int status);
```
exit()执行的动作：
- 调用注册的退出处理程序（通过atexit()和on_exit()注册的函数），其执行顺序与注册顺序相反（函数栈）
- 刷新stdio流缓冲区
- 使用由status提供的值执行_exit()系统调用。

## _exit()
```
    void _exit(int status);
```
_exit()是系统调用，即其直接陷入内核，执行内核退出进程的操作（该操作无论是进程正常终止还是异常终止都会执行）

## 内核退出进程的操作
- 关闭文件描述符（释放任何文件锁），目录流
- 分离共享内存段
- 信号量
- 取消该进程调用mmap所创建的任何内存映射
- 等等。。。

## 使用atexit注册退出处理程序

# 环境变量
```
int putenv(char *string);
int setenv(const char * name, const char *value, int overwrite);
```

关键注意点：
- putenv直接使用的是string的内存空间，即需要保证string指向的变量长期存在，全局变量或动态内存等；setenv不存在这个问题，它会做相应的拷贝。
- putenv参数形式，string的格式是“名字=值”；setenv参数分开

# 内存问题
## 慎用realloc
## 防止内存越界的几个新旧函数
**使用strncat替代strcat**
```
    char *strncat(char *dest, const char * src, size_t n);
```
从src中最多追加n个字符到dest字符串的后面。该函数自动追加‘\0’到dest的后面，所以**n应该为dest可用空间减1**。

**使用strncpy代替strcpy**
```
    char *strncpy(char *dest, const char *src, size_t n);
```
从src中最多复制n个字符到dest字符串中。需要预留‘\0’的一个字节，并手动添加‘\0’。

**使用snprintf代替sprintf**
```
    int snprintf(char *src, size_t size, const char* format, ...);
```
包含‘\0’，最多复制size个字符。

**使用fgets代替gets**
```
    char *gets(char *str);
    char *fgets(char *s, int size, FILE *stream);
```
gets函数从来不检查缓冲区的大小。fgets最多会复制size-1字节到缓冲区s中，并且会在最后一个字符后面自动追加‘\0’。

## 如何定位内存问题——Valgrind
常见内存问题：
- 动态内存泄露:malloc,free
- 资源泄露，如文件描述符
- 动态内存越界
- 数组越界
- 动态内存double free
- 使用野指针

# “长跳转”longjmp
goto是在函数内部实现短跳转，要实现跨函数跳转，得使用长跳转longjmp。

## longjmp的原理
和进程切换、信号中断返回的思想类似，要想实现非正常执行流的跳转，就需要进程在某一时刻的上下文。只要有进程上下文，将其装填到寄存器中，就能够实现跳转。
```
    int setjmp(jmp_buf env);
    void longjmp(jmp_buf env, int val);
```
- setjmp使用jmp_buf结构体保存调用时刻的进程上下文，此结构变量需保证在longjmp时时存在的。
- setjmp返回0时，为直接返回结果；返回非0值，为longjmp恢复栈空间返回的结果。
- 跳转一次之后，env保存的上下文就失效了。

## longjmp的陷阱
长跳转的实现原理是对与栈相关的寄存器的保存和恢复。

**全局变量和static变量**

保存在静态存储区，所以longjmp之后不会改变，不影响；

**局部变量**

满足一下条件的局部变量的值是不不确定的
- 调用setjmp所在函数的局部变量
- 其值在setjmp和longjmp之间有变化
- 没有被声明为volatile变量

**C++析构函数**

调用longjmp之后，没有正常解栈，本该调用的析构函数没有调用。如：
```
    void func()
    {
        Test test;
        longjmp(g_stack_env);
    }
    /*
        对象test的析构函数没有调用。
    */
```