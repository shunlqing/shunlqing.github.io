---
title: 理解右值
tag:
    -C++
categorires:
    -编程语言
---

# 概述
本文关注内容：右值和右值引用，以及移动语义、完美转发。

# 发现身边的右值

学习右值之所以会觉得很晦涩难懂，是因为我们完全习惯使用了左值。就像突然哪天告诉你，有“暗物质”的存在，你的“常识世界”是一下子不能接受的。

变量（或者说对象）可以看作是内存中的一小段。程序不断运行达到某种功能都是对若干个位于内存的对象，不断加工、修改的，各个对象又相互联系的结果。

在编程过程中，我们会定义各种变量：
```
    int i = 0;

    class ObjectA {
        ObejctA() {};
    };

    ObjectA a();
```
这些定义操作，编译器会这样做：
- 1.分配一段内存，命名为i，内存大小按照int类型标准分配，然后这段内存初始化为0；
- 2.分配一段内存，命名为a，内存大小按照ObjectA类型标准分配，然后这段内存根据默认构造函数初始化。
这样，在程序的其他地方，我们可以使用变量名i或者a，来达到修改它们所代表的内存的目的。比如：
```
    i = 1;
```
这就是左值。它们是可以取地址的。它们会有个名字，就像门牌号。程序员可以明确地说：我就要使用某某地址上的那个变量i，我要把它改成1。

那么，内存世界里，除了程序员明确定义的变量占据内存，是否还存在其他构建了的内存对象？答案是有的。在程序编译期间和运行期间，会在内存构造很多没有名字的对象（匿名对象），它们有的为了计算，有的为了支持上层的语言特性。看以下几种情景：
```
    int i = GetValue();
```
运行过程：程序通过GetValue()计算得出一个“结果”，然后将这个结果拷贝到i所命名的内存位置。我们考察这个“结果”（临时值），它同样需要一个和i一样的内存，只是在拷贝给i之后，这段内存就销毁了。

```
struct ObjectA{
    ObjectA(){
        cout << "Constructor" << endl;
    }

    ObjectA(const ObjectA& ) {
        cout << "Copy constructor" << endl;
    }

    ~ObjectA() {
        cout << "Destructor" << endl;
    }
};

int main()
{
    ObjectA a = ObjectA();
}
```
使用-fno-elide-constructors来取消返回值优化，以便观察临时值的产生。运行结果：

    Constructor
    Copy constructor
    Destructor
    Destructor

可以看出，这里构造了一个临时对象，然后拷贝给a之后，临时对象就销毁了。

此外，lambda表达式也属于右值，它们在内存中也会以某种对象的形式存在，但是你无法知悉它们的存储位置（获取地址）。

# 知道右值的存在，我们如何使用右值？

    答案：右值引用。

和左值引用一样，**右值引用允许你直接引用匿名对象的那段内存，虽然你无法知悉它的地址，但不影响你使用**。编译器使用&&符号表示右值引用。
```
    ObjectA &&a = ObjectA(); //语义：直接把临时构造的对象返回给a，a作为这个临时对象的引用供程序员使用。
```
这里运行结果：

    Constructor
    Destructor

```
    auto f = [](){ return 1; }; //lambda表达式（匿名表达式）可以使用一个右值引用去“承接”它。
    f(); //然后就可以使用这个右值引用，在其他地方调用这个lambda表达式
```

这里要区分好右值和右值引用。右值是一种对象的概念，而右值引用和左值引用一样，是引用。

# 使用右值，好处多多

通过前面的描述我们可以看到，很多时候，匿名对象（临时对象）的构造是不为人所知的。很多情况下，匿名对象的生命周期很短。它们被构造，然后在短时间内又被销毁。如果构造对象涉及开销比较大的操作，比如malloc。

我们来看一个配备移动构造函数的类对象的例子。

```
//没有移动构造函数
class ObjectA {
public:
    ObjectA() : m_ptr(new char[1]) {
        cout << "Constructor" << endl;
    }

    ObjectA(const ObjectA& a) : m_ptr(new char[*a.m_ptr]) //深拷贝
    {
        cout << "Copy Constructor" << endl;
    }

    ~ObjectA() {
        delete[] m_ptr;
        cout << "Destructor" << endl;
    }

private:
    char* m_ptr;
};

int main()
{
    ObjectA a = ObjectA(); //产生临时对象，该条语句运行完，临时对象销毁
}
```

    Constructor
    Copy Constructor
    Destructor
    Destructor

临时对象分配了动态内存，然后短时间内又析构释放。为了定义一个对象，总共发生了两次的new操作。

```
//配备移动构造函数
class ObjectA {
    // ....

    ObjectA(const ObjectA& a) : m_ptr(new char[*a.m_ptr])
    {
        cout << "Copy Constructor" << endl;
    }

    //移动构造函数
    ObjectA(ObjectA && a) : m_ptr(a.m_ptr) { //
        a.m_ptr = nullptr;
        cout << "Move Constructor" << endl;
    }
    // ...
}
```

    Constructor
    Move Constructor
    Destructor
    Destructor

只发生一次new操作。我们将临时对象new的动态内存直接转移给了返回的对象。节省开销。

程序员明确知道，会产生临时变量，而且这个临时变量会在短时间内析构。那么何不将其捕捉，直接利用呢？移动构造函数就是利用这一点。

# 左值也可转化为右值——std::move()

左值是一种对象的概念，右值也是一种对象的概念。说白了，就是内存嘛。那么，一个左值可以用右值的方式看待，从而使用到右值的一些便利呢。答案是可以。

比如，我明确清楚某个对象我不会再使用了，但是里面的资源释放掉蛮可惜的，我希望新的对象或者其他对象直接来承接这些资源。**std::move()**模板函数提供了这样的功能。

```
int main()
{
    ObjectA a = ObjectA();
    
    ObjectA b(std::move(a)); //我们明确不会再使用a,就可以将其所只有的资源转移给新的对象。（具体转移操作由移动拷贝构造完成）
}
```
运行结果：
    Constructor
    Move Constructor
    Destructor
    move ...
    Move Constructor
    Destructor
    Destructor

类似具有**转移语义**的函数还有移动赋值语句
```
    ObjectA& operator=(ObjectA&& a) {
        cout << "operator= &&" << endl;
        m_ptr = a.m_ptr;
        a.m_ptr = nullptr;
    }
```

# 完美转发 --std::forward模板

基本：一个&&的形参可以匹配左值，也可以匹配右值。&&称为universal reference。

需求：函数模板中，需要将参数转发给函数函数模板中调用的另一个函数。传入右值类型，转发的也要保留右值类型，传入左值类型，转发也要保留左值类型。

```
    void process(int& i) { cout << "lvalue call, i = " << i << endl; }
    void process(int&& i) { cout << "rvalue call, i = " << i << endl; }

    template <typename T>
    void func(T&& t)
    {
        process(t); // t是右值引用，但t本身是左值，所以匹配到的是process(int& i)
    }

    int main()
    {
        int a = 2;
        func(a); 

        func(10); 
    }
```
运行结果：

    lvalue call, i = 2
    lvalue call, i = 10

```
    template <typename T>
    void func(T&& t)
    {
        process(std::forward<T>(t)); // t是右值引用，但t本身是左值，所以匹配到的是process(int& i)
    }
```
运行结果：

    lvalue call, i = 2
    rvalue call, i = 10

**使用完美转发和移动语义来实现一个泛型的工厂函数**，这个工厂函数可以创建所有类型的对象
```
    template <typename... Args>
    T* Instance(Args... args)
    {
        return new T(std::forward<Args>(args)...);
    }
```

[TODO]
- std::move的实现
- std::forward的实现