# dict

# 目录

因为 **PyDictObject** 比其他的基本对象稍微复杂一点点，我不会在这里一步一步的展示 `__setitem__`/`__getitem__` 的过程，这些基本步骤会穿插在一些概念的解释里面一起说明

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
	* [combined table && split table](#combined-table-&&-split-table)
	* [indices and entries](#indices-and-entries)
* [哈希碰撞与删除](#哈希碰撞与删除)
* [表扩展](#表扩展)
* [类型可变的indices数组](#类型可变的indices数组)
* [free list](#free-list)
* [删除操作](#删除操作)
	* [为什么标记成 DKIX_DUMMY](#为什么标记成-DKIX_DUMMY)
	* [entries 中的删除](#entries-中的删除)
* [结束](#结束)


# 相关位置文件
* cpython/Objects/dictobject.c
* cpython/Objects/clinic/dictobject.c.h
* cpython/Include/dictobject.h
* cpython/Include/cpython/dictobject.h


# 内存构造

在深入看python 字典对象的内存构造之前，我们先来想象一下，如果让我们自己造一个字典对象会长成什么样呢？

通常情况下，我们会用哈希表来实现一个字典对象，你只用花O(1)的时间即可以完成对一个元素的查找，这也是 CPython 的实现方式

下图是在 python3.6 版本之前，一个指向 python 内部字典对象的入口指针

![entry_before](https://img-blog.csdnimg.cn/20190311111041784.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

如果你有很多稀疏的哈希表，会浪费很多内存空间，为了用更紧凑的方式来实现哈希表，可以把 哈希索引 和 真正的键值对分开存放，下图是 python3.6 版本以后的实现方式

![entry_after](https://img-blog.csdnimg.cn/20190311114021201.png)

只花费了之前差不多一半的内存，就存下了同样的一张哈希表，而且我们可以在 resize 时保持键值对的顺序，需要更详细的内容，请参考 [python-dev](https://mail.python.org/pipermail/python-dev/2012-December/123028.html) 和 [pypy-blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html)

现在，我们来看看 **PyDictObject** 的内存构造

![memory layout](https://img-blog.csdnimg.cn/20190308144931301.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

## combined table && split table

第一次看到 **PyDictObject** 的定义是很懵逼的，**ma_values** 是什么? 怎么 **PyDictKeysObject** 和上面看到的 indices/entries 结构长得不太一样?

源代码是这么注释的，我给翻译一下

    /*
    字典对象可以按照以下两种方式来表示
    The DictObject can be in one of two forms.

    Either:
      要么是一张 combined table
      A combined table:
        此时 ma_values 为 NULL, dk_refcnt 为 1
        ma_values == NULL, dk_refcnt == 1.
        哈希表的值都存储在 PyDictKeysObject 所包含的 me_value 这个字段里，也就是上图的方式存储
        Values are stored in the me_value field of the PyDictKeysObject.
    Or:
      要么是一张 split table
      A split table:
        此时 ma_values 不为空，并且 dk_refcnt 的值 大于等于 1
        ma_values != NULL, dk_refcnt >= 1
        哈希表的值都存储在 PyDictObject 里面的 ma_values 这个数组里
        Values are stored in the ma_values array.
        只能存储 unicode 对象
        Only string (unicode) keys are allowed.
        所有共享的 哈希键值 必须是按照同样顺序插入的
        All dicts sharing same key must have same insertion order.
    */

什么意思呢? 在什么情况下字段对象会共享同样的键，但是不共享值呢? 而且键还只能是 unicode 对象，字典对象中的键还不能被删除? 我们来试试看

    # 我更改了部分源代码，解释器执行 split table 查询时会打印出一些信息

    class B(object):
    	b = 1

    b1 = B()
    b2 = B()

	# __dict__ 对象还未生成
    >>> b1.b
    1
    >>> b2.b
    1

	# __dict__ 对象已经生成(通过 descriptor protocol), b1.__dict__ 和 b2.__dict__ 都是 split table, 他们共享一份 PyDictKeysObject
    >>> b1.b = 3
    in lookdict_split, address of PyDictObject: 0x10bc0eb40, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    >>> b2.b = 4
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    >>> b1.b
    in lookdict_split, address of PyDictObject: 0x10bc0eb40, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    3
    >>> b2.b
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    4
    # 进行删除操作之后，b2.__dict__ 会从 split table 变为 combined table
    >>> b2.x = 3
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: x
    >>> del b2.x
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: x
    # 现在，看不到 lookdict_split 的输出了，说明已经变成了 combined table
	>>> b2.b
	4

split table 可以在你对同个class有非常多实例的时候节省很多内存，这些实例在满足上述条件时，都会共享同一个 PyDictKeysObject, 更多关于实现方面的细节请参考 [PEP 412 -- Key-Sharing Dictionary](https://www.python.org/dev/peps/pep-0412/)

![dict_shares](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_shares.png)

## indices and entries

我们进到源代码里看一下 cpython 如何在 **PyDictKeysObject** 中实现 indices/entries, **char dk_indices[]** 又是什么意思?

    /*
    这是代码本身的注释，我来翻译一下
    dk_indices 是真正的哈希表，他在 entries 存储了哈希的索引，或者 DKIX_EMPTY(-1) 和 DKIX_DUMMY(-2) 的一种
    dk_indices is actual hashtable.  It holds index in entries, or DKIX_EMPTY(-1)
    or DKIX_DUMMY(-2).
    dk_size 字段表示 indices 的大小，但是这个字段的类型是可以根据表的大小变化的
    Size of indices is dk_size.  Type of each index in indices is vary on dk_size:

    * int8  for          dk_size <= 128
    * int16 for 256   <= dk_size <= 2**15
    * int32 for 2**16 <= dk_size <= 2**31
    * int64 for 2**32 <= dk_size

	dk_entries 是一个数组，里面的对象类型是 PyDictKeyEntry, 他的大小可以用 USABLE_FRACTION 这个宏来表示， DK_ENTRIES(dk) 可以获得真正哈希键对值的第一个入口
    dk_entries is array of PyDictKeyEntry.  It's size is USABLE_FRACTION(dk_size).
    DK_ENTRIES(dk) can be used to get pointer to entries.

	# 注意, DKIX_EMPTY 和 DKIX_DUMMY 是用负数表示的，所有 dk_indices 里面的索引类型也需要用有符号整数表示，int16 用来表示 dk_size 为 256 的表
    NOTE: Since negative value is used for DKIX_EMPTY and DKIX_DUMMY, type of
    dk_indices entry is signed integer and int16 is used for table which
    dk_size == 256.
    */

    #define DK_SIZE(dk) ((dk)->dk_size)
    #if SIZEOF_VOID_P > 4
    #define DK_IXSIZE(dk)                          \
        (DK_SIZE(dk) <= 0xff ?                     \
            1 : DK_SIZE(dk) <= 0xffff ?            \
                2 : DK_SIZE(dk) <= 0xffffffff ?    \
                    4 : sizeof(int64_t))
    #else
    #define DK_IXSIZE(dk)                          \
        (DK_SIZE(dk) <= 0xff ?                     \
            1 : DK_SIZE(dk) <= 0xffff ?            \
                2 : sizeof(int32_t))
    #endif
    #define DK_ENTRIES(dk) \
        ((PyDictKeyEntry*)(&((int8_t*)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]))

这个宏一开始看的时候不是很好理解，我们把他重写一下

    #define DK_ENTRIES(dk) \
        ((PyDictKeyEntry*)(&((int8_t*)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]))

重写成

    // 假设 indices array 里的类型为 int8_t
    size_t indices_offset = DK_SIZE(dk) * DK_IXSIZE(dk);
    int8_t *pointer_to_indices = (int8_t *)(dk->dk_indices);
    int8_t *pointer_to_entries = pointer_to_indices + indices_offset;
    PyDictKeyEntry *entries = (PyDictKeyEntry *) pointer_to_entries;

现在就很清晰了

![dictkeys_basic](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dictkeys_basic.png)

# 哈希碰撞与删除

cpython 是怎么处理字典对象里的哈希碰撞的呢? 除了依靠一个好的哈希函数，cpython 还依赖 perturb 策略，
我们来读一读源代码看看


	j = ((5*j) + 1) mod 2**i
    0 -> 1 -> 6 -> 7 -> 4 -> 5 -> 2 -> 3 -> 0 [and here it's repeating]
    perturb >>= PERTURB_SHIFT;
    j = (5*j) + 1 + perturb;
    use j % 2**i as the next table index;

我更改了部分源代使得每次打印出更多信息


    >>> d = dict()
    >>> d[1] = 1
    : 1, ix: -1, address of ep0: 0x10870d798, dk->dk_indices: 0x10870d790
    ma_used: 0, ma_version_tag: 11313, PyDictKeyObject.dk_refcnt: 1, PyDictKeyObject.dk_size: 8, PyDictKeyObject.dk_usable: 5, PyDictKeyObject.dk_nentries: 0
    DK_SIZE(dk): 8, DK_IXSIZE(dk): 1, DK_SIZE(dk) * DK_IXSIZE(dk): 8, &((int8_t *)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]): 0x10870d798

	>>> repr(d)
    ma_used: 1, ma_version_tag: 11322, PyDictKeyObject.dk_refcnt: 1, PyDictKeyObject.dk_size: 8, PyDictKeyObject.dk_usable: 4, PyDictKeyObject.dk_nentries: 1
    index: 0 ix: -1 DKIX_EMPTY
    index: 1 ix: 0 me_hash: 1, me_key: 1, me_value: 1
    index: 2 ix: -1 DKIX_EMPTY
    index: 3 ix: -1 DKIX_EMPTY
    index: 4 ix: -1 DKIX_EMPTY
    index: 5 ix: -1 DKIX_EMPTY
    index: 6 ix: -1 DKIX_EMPTY
    index: 7 ix: -1 DKIX_EMPTY
	'{1: 1}'

这里 indices 是索引, 索引里面 **-1(DKIX_EMPTY)** 表示这个坑位是空的, 现在代码里设置了 d[1] = 1, hash(1) & mask == 1, 会对应到 indices 的下标为 1 的坑位上, 这个坑位原本是 -1(DKIX_EMPTY) 表示没有被占用过, 马上占用他, 把这里的 -1 改成 entries 里面第一个空余的真正的存储 key 和 value 的位置, 这个位置是 0, 所以就把 0 存储到了 indices[1] 里

![hh_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_1.png)

    d[4] = 4

![hh_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_2.png)

    d[7] = 111

![hh_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_3.png)

	# 删除的时候并不会把索引清除，而是把对应那格的索引标记成 DKIX_DUMMY, DKIX_DUMMY 数字表示为 -2
    # 并且 dk_usable 和 dk_nentries 并没有改变
    del d[4]

![hh_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_4.png)

	# 注意, dk_usable 和 dk_nentries 现在变了
	d[0] = 0

![hh_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_5.png)

	d[16] = 16
    # hash (16) & mask == 0
    # 但是索引位置 0 已经被 key: 0, value: 0 这个对象占用了
    # 当前 perturb = 16, PERTURB_SHIFT = 5, i = 0
    # 所以, perturb >>= PERTURB_SHIFT ===> perturb == 0
    # i = (i*5 + perturb + 1) & mask ===> i = 1
    # 现在, 索引位置 1 依然被 key: 1, value: 1 占用
    # 当前 perturb = 0, PERTURB_SHIFT = 5, i = 1
    # 所以, perturb >>= PERTURB_SHIFT ===> perturb == 0
    # i = (i*5 + perturb + 1) & mask ===> i = 6
    # 此时索引位置 6 是空的，插入

![hh_6](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_6.png)

# 表扩展

	# 现在, dk_usable 为 0, dk_nentries 为 5
    # 再插入一个对象试试
    d[5] = 5
    # 第一步，表达到了阈值，扩展表，并复制
    # 索引里被标记成 DKIX_DUMMY 不会被复制，所以索引对应的位置后面的元素都会往前移

![resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/resize.png)

	# 第二步， 插入 key: 5, value: 5

# 类型可变的indices数组
indices 数组的大小是可变的，当你的哈希表大小 <= 128 时，索引数组的元素类型为 int_8, 表变大时会用 int16 或者 int64 来表示，这样做可以节省内存使用，这个策略在上面代码注释部分说明过了

# free list

	static PyDictObject *free_list[PyDict_MAXFREELIST];

cpython 也会用 free_list 来重新循环使用那些被删除掉的字典对象，可以避免内存碎片和提高性能，需要图解的同学可以参考 [list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_cn.md#%E4%B8%BA%E4%BB%80%E4%B9%88%E7%94%A8-free-list), cpython set 使用了同样的策略，里面有图片解释

# 删除操作

字典中元素的删除使用的是 [lazy deletion](https://en.wikipedia.org/wiki/Lazy_deletion) 策略(上面已展示过)

## 为什么标记成 DKIX_DUMMY

indices 总共只有三种不同状态的值, **DKIX_EMPTY**(-1), **DKIX_DUMMY**(-2) 和 **Active/Pending**(>=0). 如果你把删除的对象标记为 **DKIX_EMPTY** 而不是 **DKIX_DUMMY**, **perturb** 策略在插入/搜索这个对象的时候将会出现问题

假设你有一个大小为 8 的哈希表, 他的 indices 如下所示

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [3]  [4]
    index:    0    1         2      3        4           5          6    7

当你搜索一个哈希值为 0 的对象, 并且这个对象的位置在 entries[4] 时, "perturb" 的搜索过程如下

	0 -> 1 -> 6 -> 7(找到匹配的值, 不需要往下搜索了)

假设你删除掉 indices[6] 上面的对象, 并把他标记为 **DKIX_EMPTY**, 当你再次搜索同一个对象的时候

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [DKIX_EMPTY]  [4]
    index:    0    1         2      3        4           5               6        7

搜索过程如下所示

	0 -> 1 -> 6(这个位置上的值为 DKIX_EMPTY, 最后一个哈希值相同的对象插入的时候只会插入到上一个perturb的位置, 也就是 indices[1], 如果搜索到第一个 DKIX_EMPTY 还没有发现匹配的值, 说明搜索的这个值不在这张表里)

实际上需要搜索的对象在 indices[7] 上面, 但是由于错误的标记了 **DKIX_EMPTY**, 搜索停留在了 indices[6] 上, 如果我们把他标记成正确的值 **DKIX_DUMMY**

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [DKIX_DUMMY]  [4]
    index:    0    1         2      3        4           5               6        7

搜索过程会变成

	0 -> 1 -> 6(DKIX_DUMMY, 说明这个坑位之前被插入过, 但已经被删除, 也许后面还有同样的哈希值的对象, 继续往下搜索) -> 7(找到匹配的值, 不需要往下搜索了)

当然, 标记为 **DKIX_DUMMY** 的坑位也可以用来插入新的对象

## entries 中的删除

dict 对象需要保证字典中元素按照 [插入顺序](https://mail.python.org/pipermail/python-dev/2017-December/151283.html) 来进行保存, 删除操作不能对 **entries** 中的元素进行排序

把 entries[i] 当成空的值, 在后续表扩展/压缩时再重新压缩这部分空余的空间, 这样做可以保持删除操作的平均时间复杂度为 O(1)

并且通常情况下字典对象的删除操作并不普遍, 只有部分情况下会导致性能变慢([PyPy Status Blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html))

#### 结束

现在，我们大致弄明白了 cpython 字典对象的内部实现了!