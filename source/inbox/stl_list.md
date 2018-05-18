---
title: STL list
tags:
    - C++
    - STL
categories:
    - 编程语言
---

# STL list结构

    环状双向链表

    空list有一个空结点，以支持左闭右开的迭代器

# insert

    规定：在pos之前插入

    insert(iterator pos, T&& value); //怎么实现