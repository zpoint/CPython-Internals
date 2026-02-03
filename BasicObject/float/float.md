# float

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
    * [0](#0)
    * [1](#1)
    * [0.1](#0.1)
    * [1.1](#1.1)
    * [-0.1](#-0.1)
* [free_list](#free_list)

# related file
* cpython/Objects/floatobject.c
* cpython/Include/floatobject.h
* cpython/Objects/clinic/floatobject.c.h

# memory layout

**PyFloatObject** is simply a wrapper around the C type **double**, which takes 8 bytes to represent a floating-point number

you can refer to [IEEE 754](https://en.wikipedia.org/wiki/IEEE_754-1985)/[IEEE-754标准与浮点数运算](https://blog.csdn.net/m0_37972557/article/details/84594879) for more detail

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/layout.png)

# example

## 0

The binary representation of 0.0 in **IEEE 754** format is 64 zero bits

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

The difference between 1.1 and 0.1 is the last few exponent bits

![1.1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/1.1.png)

## -0.1

The difference between -0.1 and 0.1 is the first sign bit

![-0.1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/-0.1.png)

# free_list

```c
#ifndef PyFloat_MAXFREELIST
#define PyFloat_MAXFREELIST    100
#endif
static int numfree = 0;
static PyFloatObject *free_list = NULL;

```

free_list is a singly linked list that stores at most PyFloat_MAXFREELIST **PyFloatObject**s

![free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/free_list.png)

The linked list is linked via the **ob_type** field

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

**f3** comes from the head of the **free_list**

```python3
>>> f3 = 3.0
>>> id(f3)
4551393616

```

![free_list3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/free_list3.png)
