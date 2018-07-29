---
title: new和malloc
tags:
    - C++
categories:
    - 编程语言
---

刚学习new和delete的时候，我们就被忠告：

    new和delete，new[]和delete[]要配对使用，不能混淆使用。

具体是为什么呢？我们都知道new和malloc都是用来分配内存的。那么他们都有什么区别呢？简单来说，malloc仅仅是分配指定大小的内存，而new不仅分配了内存，还在内存上面构造了对象。好比如，你去申请一块100平的地建房子，malloc就真的只是给你一块100平方的地，上面什么也没有；而new不仅给你100平方的地，而且按照你的意愿，已经在上面改好了房子，你直接入住就可以了。如果是new[]操作，就像是给你建好了一排别墅的感觉。

## new和malloc

对于内置类型：

```cpp
#include <stdlib.h>
int main()
{
    int* p1 = (int*)malloc(sizeof int);
    int* p2 = new int();
}
```
对于内置类型，malloc和new都是分配了指定大小的内存，然后简单初始化为0。内置类型没有构造函数，也不需要调用。

对于没有自定义构造函数的类型：

```cpp
#include <stdlib.h>
class Test {
public:
    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test));
    Test* p2 = new Test();
}
```

对于没有自定义构造函数的类型来说，malloc和new也是分配了指定类对象大小的内存，malloc简单初始化为0（可能有的甚至不做初始化），new也是这样么？我们为Test类增加一个虚函数。
```cpp
#include <stdlib.h>
class Test {
public:
    virtual void foo(){};
    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test));
    Test* p2 = new Test();
    // p1->foo(); 会发生Segmentation fault
    p2->foo();
}
```

虽然没有自定义构造函数，但是还是看出差别，因为有虚函数的类对象需要有一个虚表指针指向虚表，用于在运行期动态获取虚函数的地址，new不仅分配了对象内存，而且并在对象内存的特定位置构造好了虚表指针，malloc根本不会这样做。

```cpp
#include <stdlib.h>
#include <iostream>
class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }

    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test));
    std::cout << "---------" << std::endl;
    Test* p2 = new Test();
}
```
执行结果：

    ---------
    Test()

可见，对于有自定义构造函数的类来说，new会调用自定义构造函数。

## new[]和malloc

对于new[]和malloc分配内置类型或没有自定义构造函数类的对象数组，其差别基本跟上面所讲的差不多。

1. 对于有自定义构造函数的类，new[]需要调用相应次数的构造函数。
```cpp
#include <stdlib.h>
#include <iostream>
class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }
    ~Test() {
        std::cout << "~Test()" << std::endl;
    }
    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test)* 5);
    std::cout << "---------" << std::endl;
    Test* p2 = new Test[5]();
}
```

    ---------
    Test()
    Test()
    Test()
    Test()
    Test()

2. 有自定义析构函数和没有自定义析构函数的区别

测试环境是：Ubuntu 16.04 64位

```cpp
#include <stdlib.h>
#include <iostream>
class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }
    ~Test() {
        std::cout << "~Test()" << std::endl;
    }
    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test)* 5);
    std::cout << "---------" << std::endl;
    Test* p2 = new Test[5]();
    //delete p2; //用delete代替delete[], 产生coredump
    delete (char*)p2 - 8; //不会core dump，但是不会执行析构函数。
    delete p1;
}
```
使用valgrind检测，发现，`delete ((char*)p2 - 8)`不会出现内存泄漏，也就是说已经正常回收。

```cpp
// 没有自定义析构函数的情况下
#include <stdlib.h>
#include <iostream>
class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }
    // ~Test() {
    //     std::cout << "~Test()" << std::endl;
    // }
    int a;
};

int main()
{
    Test* p1 = (Test*)malloc(sizeof(Test)* 5);
    std::cout << "---------" << std::endl;
    Test* p2 = new Test[5]();
    delete p2; //不会产生core dump
    // delete (char*)p2 - 8; //会core dump
}
```

我们看下，在有自定义析构函数的情况下，多出来的8字节（64位机）是干嘛的。
```cpp
#include <stdlib.h>
#include <iostream>
class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }
    ~Test() {
        std::cout << "~Test()" << std::endl;
    }
    int a;
};

int main()
{
    Test* p2 = new Test[5]();
    std::cout << *(int*)((char*)p2-8) << std::endl;
    delete[] p2;
}
```

    Test()
    Test()
    Test()
    Test()
    Test()
    5
    ~Test()
    ~Test()
    ~Test()
    ~Test()
    ~Test()

存储了数字刚好是数组的大小。由此我们推测，这不仅仅是记录数组存储对象的个数，因为个数本身没什么意义。更重要的是，**要正确执行相应次数的析构函数**，这也是为什么没有自定义构造函数就不需要记录这个数字，使用`delete ptr-8`(64位机)会引发core dump。

所以delete和delete[]最好不要混用。

