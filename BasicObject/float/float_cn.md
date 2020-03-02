# float

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
	* [0](#0)
	* [1](#1)
	* [0.1](#0.1)
	* [1.1](#1.1)
	* [-0.1](#-0.1)
* [free_list](#free_list)

# 相关位置文件
* cpython/Objects/floatobject.c
* cpython/Include/floatobject.h
* cpython/Objects/clinic/floatobject.c.h

# 内存构造

**PyFloatObject** 仅仅是一层对 c 语言中双精度浮点数的包装(**double**), 一个双精度浮点数使用8个字节去表示一个浮点数

详细的内容可以参考 [IEEE 754](https://en.wikipedia.org/wiki/IEEE_754-1985)/[IEEE-754标准与浮点数运算](https://blog.csdn.net/m0_37972557/article/details/84594879)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/layout.png)

# 示例

## 0

0.0 使用 **IEEE 754** 标准的表示方式为 64 个为 0 的 bit

```python3
f = 0.0

```

![0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/0.png)

## 1.0

```python3
f = 1.0

```

![1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/1.png)

## 0.1

```python3
f = 0.1

```

![0.1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/0.1.png)

## 1.1

1.1 和 0.1 的区别是指数位最后的几个位不相同

![1.1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/1.1.png)

## -0.1

-0.1 和 0.1 的区别是第一个符号位不相同

![-0.1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/-0.1.png)

# free_list

```c
#ifndef PyFloat_MAXFREELIST
#define PyFloat_MAXFREELIST    100
#endif
static int numfree = 0;
static PyFloatObject *free_list = NULL;

```

free_list 是一个单链表, 最多存储 **PyFloat_MAXFREELIST** 个 **PyFloatObject**

![free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/free_list.png)

单链表通过 **ob_type** 字段串联起来

```python3
>>> f = 0.0
>>> id(f)
4551393664
>>> f2 = 1.0
>>> id(f2)
4551393616
del f
del f2

```

![free_list2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/free_list2.png)

**f3** 取自 **free_list** 的表头

```python3
>>> f3 = 3.0
>>> id(f3)
4551393616

```

![free_list3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/free_list3.png)
