---
title: STL_Vector
tags:
    - C++
    - STL
categories:
    - 编程语言
---

# vector的增长速度？

# resize() reserve() shrink_to_fit()
## resize(new_size)

对已经构造部分的大小进行重新整理。
- new_size > 当前大小： 新构造部分，由T()默认填充
- new_size < 当前大小： 截断已构造部分，销毁

## reserve(new_cap)

调整容量：
- new_cap < 当前容量大小：什么也不做
- new_cap > 当前容量大小：重新分配，移动元素

**好习惯：**
在定义vector对象的时候，如果已经知道vector对象短时间内会增加到什么样的规模，可以提前调用reserve().以免在push_back()的过程中，反复重新分配，效率低下。


## shrink_to_fit()

语义：请求调整当前容量到size的大小。（即把当前多余未构造的空间还给分配器）。
- 怎么实现：不能只是部分返还，所以要重新分配一个较小的空间（size()大小），然后拷贝。

# insert最复杂
