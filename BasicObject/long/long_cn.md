# int![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/long_cn)

感谢 @MambaWong 指出之前本章节存在的问题 [#22](https://github.com/zpoint/CPython-Internals/issues/22) 已改正

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [内部元素如何存储](#内部元素如何存储)
	* [整数 0](#整数-0)
	* [整数 1](#整数-1)
	* [整数 -1](#整数-1)
	* [整数 1023](#整数-1023)
	* [整数 32767](#整数-32767)
	* [整数 32768](#整数-32768)
	* [小端大端](#小端大端)
	* [第一个保留位](#第一个保留位)
* [small ints(缓冲池)](#small-ints)

# 相关位置文件
* cpython/Objects/longobject.c
* cpython/Include/longobject.h
* cpython/Include/longintrepr.h

# 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190314164305131.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

在 python3 之后, 就只有一个叫做 **int** 的类型了, python2.x 的 **long** 类型在 python3.x 里面就叫做 **int** 类型

**int** 在内存空间上的构造和 [tuple 元素在内存空间的构造](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md#%E5%86%85%E5%AD%98%E6%9E%84%E9%80%A0) 非常相似

很明显, 只有 **ob_digit** 这一个位置可以用来存储真正的整数, 但是 cpython 如何按照字节来存储任意长度的整型的呢?

我们来看看

# 内部元素如何存储

## 整数 0

注意, 当要表示的整数的值为 0 时, **ob_digit** 这个数组为空, 不存储任何东西, **ob_size** 中的 0 就直接表示这个整数的值为 0, 这是一种特殊情况

```python3
i = 0

```

![0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/0.png)

## 整数 1

**ob_digit** 可以有两种不同的定义, 具体是 uint32_t 还是 unsigned short 取决于操作系统

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

我把源代码里的 **PYLONG_BITS_IN_DIGIT** 的值写死成了 15, 这样我下面所有的示例都是以 **unsigned short** 定义的 **digit**

当我们需要表示整数 1 的时候, **ob_size** 的值变成了1, 这个时候 **ob_size** 表示 **ob_digit** 的长度, 并且 **ob_digit** 里以 **unsigned short** 的表示方式存储了整数1

```python3
i = 1

```

![1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/1.png)

## 整数 -1

当 i 变成 -1 时候, 唯一的和整数 1 的区别就是储存在 **ob_size** 里的值变成了 -1, 这里的负号表示这个整数的正负性, 不影响到 **ob_digit** 里面的值

```python3
i = -1

```

![-1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/-1.png)

## 整数 1023

对于 PyLongObject 来说, 最基本的存储单位是 **digit**, 在我这里是 2个 byte 的大小(16个bit). 1023 只需要占用最右边的10个bit 就够了, 所以 **ob_size** 里的值仍然是 1


![1023](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/1023.png)

## 整数 32767

整数 32767 的存储方式依旧和上面的存储无大致上的差别

![32767](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/32767.png)

## 整数 32768

我们发现, cpython 并不会占用掉一个 **digit** 的所有的 bit 去存储一个数, 第一个 bit 会被保留下来, 我们后面会看到这个保留下来的 bit 有什么作用

![32768](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/32768.png)

## 小端大端

注意, 因为 **digit** 作为 cpython 表示整型的最小存储单元, **digit** 里面的 byte 存储的顺序和你的机器的顺序一致

而 **digit** 和 **digit** 之间则是按照 权重最重的 **digit** 在最右边 原则存储的(小端存储)

我们来看一看整数值 -262143 的存储方式

负号依旧存储在 **ob_size** 里面

整数值 262143(2^18 = 262144) 的二进制表示应该是 00000011 11111111 11111111

![262143](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/262143.png)

## 第一个保留位

为什么 **digit** 的最左边一位需要保留下来呢? 为什么 **ob_digit** 里面的 **digit** 的顺序是按照小端的顺序存储的呢?

我们尝试跑一个简单的加法来理解一下

```python3
i = 1073741824 - 1 # 1 << 30 == 1073741824
j = 1

```

![before_add](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/before_add.png)

```python3
k = i + j

```

首先, 相加之前会初始化一个中间变量我这里叫做 temp, 他的类型也是 **PyLongObject**, 并且大小为 max(size(i), size(j)) + 1

![temp_add](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/temp_add.png)

第一步, 把两个数 **ob_digit** 里的第一个坑位里的 **digit** 加起来, 并加到 **carray** 里

![step_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_1.png)

第二步, 把 temp[0] 里的值设置为 (carry & PyLong_MASK)

![step_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_2.png)

第三步, 右移 carray, 值保留下最左边的一位(这一位其实就是之前两个数相加的进位)

![step_3_rshift](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_3_rshift.png)

第四步, 把两边下一个 **ob_digit** 的对应位置的值加起来, 并把结果与 **carray** 相加

![step_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_4.png)

第五步, 把 temp[1] 里的值设置为 (carry & PyLong_MASK)

![step_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_5.png)

第六步, 再次右移

![step_3_rshift](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_3_rshift.png)

回到步骤四, 直到两边都没有剩余的 **digit** 可以相加为止, 把最后的 carray 存储到 temp 最后一格

![step_final](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/step_final.png)

temp 这个变量此时按照 **PyLongObject** 的方式存储了前面两个数的和, 现在细心地你应该发现了, 第一个保留位是为了用来相加或者相减的时候作进位/退位 用的, 并且当 **digit** 和 **digit** 之间按照小端的方式存储的时候, 你做加减法的时候只需要从左到右处理即可

减法的操作和加法类似, 有兴趣的同学可以自己参考源代码

![k](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/k.png)


## small ints

cpython 同时也使用了一个全局变量叫做 small_ints 来单例化一部分常见范围的整数, 这么做可以减少频繁的向操作系统申请和释放的次数, 并提高性能

```c

#define NSMALLPOSINTS           257
#define NSMALLNEGINTS           5
static PyLongObject small_ints[NSMALLNEGINTS + NSMALLPOSINTS];

```

我们来看看

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