# int

Thanks @MambaWong  for pointing out the errors [#22](https://github.com/zpoint/CPython-Internals/issues/22) of this article

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [how is element stored inside](#how-is-element-stored-inside)
    * [ingeter 0](#ingeter-0)
    * [ingeter 1](#ingeter-1)
    * [ingeter -1](#ingeter--1)
    * [ingeter 1023](#ingeter-1023)
    * [ingeter 32767](#ingeter-32767)
    * [ingeter 32768](#ingeter-32768)
    * [little endian and big endian](#little-endian-and-big-endian)
    * [reserved bit](#reserved-bit)
* [small ints](#small-ints)

# related file
* cpython/Objects/longobject.c
* cpython/Include/longobject.h
* cpython/Include/longintrepr.h

# memory layout

![memory layout](https://img-blog.csdnimg.cn/20190314164305131.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

after python3, there's only type named **int**, the **long** type in python2.x is **int** type in python3.x

the structure of **long object** looks like the structure of [tuple object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#memory-layout), obviously, there's only one field to store the real **int** value, that's **ob_digit**

But how does CPython represent the variable size **int** in byte level? Let's see

# how is element stored inside

## ingeter 0

notice, when the value is 0, the **ob_digit** field doesn't store anything, the value 0 in **ob_size** indicate that **long object** represent integer 0

```python3
i = 0

```

![0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/0.png)

## ingeter 1

there are two different types of **ob_digit** depends on your system.

```c
#if PYLONG_BITS_IN_DIGIT == 30
typedef uint32_t digit;
typedef int32_t sdigit;
typedef uint64_t twodigits;
typedef int64_t stwodigits; /* signed variant of twodigits */
#define PyLong_SHIFT    30
#define _PyLong_DECIMAL_SHIFT   9 /* max(e such that 10**e fits in a digit) */
#define _PyLong_DECIMAL_BASE    ((digit)1000000000) /* 10 ** DECIMAL_SHIFT */
#elif PYLONG_BITS_IN_DIGIT == 15
typedef unsigned short digit;
typedef short sdigit; /* signed variant of digit */
typedef unsigned long twodigits;
typedef long stwodigits; /* signed variant of twodigits */
#define PyLong_SHIFT    15
#define _PyLong_DECIMAL_SHIFT   4 /* max(e such that 10**e fits in a digit) */
#define _PyLong_DECIMAL_BASE    ((digit)10000) /* 10 ** DECIMAL_SHIFT */

```

I've modified the source code to change the value of **PYLONG_BITS_IN_DIGIT** to 15

but when it's going to represent integer 1, **ob_size** becomes 1 and field in **ob_digit** represent the value 1 with type **unsigned short**

```python3
i = 1

```

![1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/1.png)

## ingeter -1

when i becomes -1, the only difference from the integer 1 is the value in **ob_size** field, CPython interpret **ob_size** as a signed type to differ the positive and negative sign

```python3
i = -1

```

![-1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/-1.png)

## ingeter 1023

the basic unit is type **digit**, which provide 2 bytes(16bits) for storage. And 1023 takes the rightmost 10 bits,
so the value **ob_size** field is still 1.


![1023](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/1023.png)

## ingeter 32767

the integer 32767 represent in the same way as usual

![32767](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/32767.png)

## ingeter 32768

CPython don't use all the 16 bits in **digit** field, the first bit of every **digit** is reserved, we will see why later

![32768](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/32768.png)

## little endian and big endian

notice, because the **digit** is the smallest unit in the CPython abstract level, The order between bytes inside a single ob_digit is the same as your machine order(mine is little endian)

Order between **digit** in the **ob_digit** array are represent as most-significant-digit-in-the-right-most order(little endian order)

we can have a better understanding with the integer value -262143

the minus sign is stored in the **ob_size** field

the interger 262143(2^18 = 262144) in binary representation is 00000011 11111111 11111111

![262143](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/262143.png)

## reserved bit

why the left-most bit in **digit** is reserved? Why order between **digit** in the **ob_digit** array are represented as little-endian?

let's try to add two integer value

```python3
i = 1073741824 - 1 # 1 << 30 == 1073741824
j = 1

```

![before_add](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/before_add.png)

```python3
k = i + j

```

first, initialize a temporary **PyLongObject** with size = max(size(i), size(j)) + 1

![temp_add](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/temp_add.png)

step1, sum the firt **digit** in each **ob_digit** array to a variable named **carray**

![step_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_1.png)

step2, set the value in temp[0] to (carry & PyLong_MASK)

![step_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_2.png)

step3, right shift the carray up to the leftmost bit

![step_3_rshift](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_3_rshift.png)

step4, add the second **digit** in each **ob_digit** array to the result of **carray**

![step_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_4.png)

step5, set the value in temp[1] to (carry & PyLong_MASK)

![step_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_5.png)

step6, right shift the carray again

![step_3_rshift](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_3_rshift.png)

go to step4 and repeat until no more **digit** left, set the final carray to the last index of temp

![step_final](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_final.png)

the variable temp contains the sum, now, you see the reserved bit is used for the **carry** or **borrow** when you add/sub an integer, the **digit** in **ob_digit** array are stored in little-endian order so that the add/sub operation can process each **digit** from left to right

the sub operation is similar to the add operation, so you can read the source code directly

![k](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/k.png)


## small ints

CPython also use a buffer pool to store the frequently used integer

```c

#define NSMALLPOSINTS           257
#define NSMALLNEGINTS           5
static PyLongObject small_ints[NSMALLNEGINTS + NSMALLPOSINTS];

```

let's see

```python3
c = 0
d = 0
e = 0
print(id(c), id(d), id(e)) # 4480940400 4480940400 4480940400
a = -5
b = -5
print(id(a), id(b)) # 4480940240 4480940240
f = 1313131313131313
g = 1313131313131313
print(id(f), id(g)) # 4484604176 4484604016

```

![small_ints](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/small_ints.png)