# dict

### 目录

因为 **PyDictObject** 比其他的基本对象稍微复杂一点点，我不会在这里一步一步的展示 _\_setitem_\_/_\_getitem_\_ 的过程，这些基本步骤会穿插在一些概念的解释里面一起说明

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
	* [combined table && split table](#combined-table-&&-split-table)

#### 相关位置文件
* cpython/Objects/dictobject.c
* cpython/Objects/clinic/dictobject.c.h
* cpython/Include/dictobject.h
* cpython/Include/cpython/dictobject.h


#### 内存构造

在深入看python 字典对象的内存构造之前，我们先来想象一下，如果让我们自己造一个字典对象会长成什么样呢？

通常情况下，我们会用哈希表来实现一个字典对象，你只用花O(1)的时间即可以完成对一个元素的查找，这也是 cpython 的实现方式

下图是在 python3.6 版本之前，一个指向 python 内部字典对象的入口指针

![entry_before](https://img-blog.csdnimg.cn/20190311111041784.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

如果你有很多稀疏的哈希表，会浪费很多内存空间，为了用更紧凑的方式来实现哈希表，可以把 哈希索引 和 真正的键值对分开存放，下图是 python3.6 版本以后的实现方式

![entry_after](https://img-blog.csdnimg.cn/20190311114021201.png)

只花费了之前差不多一半的内存，就存下了同样的一张哈希表，而且我们遍历哈希表的时候遍历 entries 就可以实现让字典对象维持插入/删除的顺序，这一点在之前是做不到的，因为每次哈希表扩容/缩小，都要重新哈希所有元素，顺序会打乱，现在重新哈希会打乱的只是 indices, 需要更详细的内容，请参考 [python-dev](https://mail.python.org/pipermail/python-dev/2012-December/123028.html) 和 [pypy-blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html)

现在，我们来看看 **PyDictObject** 的内存构造

![memory layout](https://img-blog.csdnimg.cn/20190308144931301.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

##### combined table && split table

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

