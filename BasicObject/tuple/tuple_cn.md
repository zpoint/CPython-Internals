# tuple

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [内部元素如何存储](#内部元素如何存储)
* [free list(缓冲池)](#free-list)

# 关位置文件
* cpython/Include/cpython/tupleobject.c
* cpython/Include/tupleobject.h
* cpython/Objects/tupleobject.c

# 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190313121821367.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

**tuple** 对象的构造比起其他对象来说相对简单一些. 从图中可以很明显的发现 **ob_item** 是一个数组，数组的元素都是 指向 PyObject 的指针，所以所有的元素都会存储在 **ob_item** 数组中,但是这些元素实际上存在哪一个格子里呢,第一个元素是从第0个格子开始存储的吗,resize 策略是怎样的呢?

让我们来研究一下

# 内部元素如何存储

	t = tuple()

![tuple_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_empty.png)

	t = ("aa", )

![tuple_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_1.png)

	t = ("aa", "bb", "cc", "dd")

**ob_size** 表示 **PyTupleObject** 的真正大小，因为 tuple 对象是不可变的，**ob_item** 这个字段的位置就是第一个 tuple 元素开始的位置，并且不需要进行 resize 操作

![tuple_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_4.png)

# free list

这里使用的 free list(缓冲池) 的机制比起 [list 对象里的 free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_cn.md#delete-%E5%92%8C-free-list) 要有趣一些

	#define PyTuple_MAXSAVESIZE     20
    #define PyTuple_MAXFREELIST  2000
    static PyTupleObject *free_list[PyTuple_MAXSAVESIZE];
	static int numfree[PyTuple_MAXSAVESIZE];

我们来验证一下 **free_list** 和 **numfree** 分别是什么
以下的图像示例是在认为 python 解释器在执行这部分代码的情况下不会自己创建/删除 新的 tuple 对象所展示的, 仅供解释使用, 实际情况要复杂一些

	>>> t = tuple()
    >>> id(t)
    4458758208
    >>> del t
    >>> t = tuple()
    >>> id(t) # 和之前删除的 id 相同
    4458758208
    >>> t2 = tuple()
    >>> id(t2) # 和 t 的 id 相同
    4458758208

![delete_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_0.png)

	>>> t = ("aa", )
    >>> id(t)
    4459413360
    >>> del t
    >>> t = ("bb", )
    >>> id(t) # 打印出的 id 和上面相同，但是这是不可重现的，实际上因为解释器运行的时候也会创建和删除相同大小的对象，所以会有偏移
    4459413360
    >>> t2 = ("cc", )
    >>> del t
    >>> del t2

![delete_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_2.png)

num_free[i] 表示 free_list[i] 当前剩下的对象的数量，当你要创建一个大小为 i 的 tuple 对象时，cpython 会重新利用挂在 free_list[i] 的最顶上的元素去(如果num_free[i]大于0的话)

	>>> t = ("aa", )

![delete_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_3.png)

	>>> t2 = ("aa", "bb")
    >>> t3 = ("cc", "dd")
    >>> t4 = ("ee", "ff")
    >>> t5 = ("gg", "hh")
    >>> del t2
    >>> del t3
    >>> del t4
    >>> del t5

![delete_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_4.png)
