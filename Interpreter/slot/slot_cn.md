# slot

# 目录

* [相关位置文件](#相关位置文件)
* [slot](#slot)
* [示例](#示例)
* [实例属性访问](#实例属性访问)
	* [访问实例属性wing](#访问实例属性wing)
		* [设置了值之前](#设置了值之前)
		* [设置了值之后](#设置了值之后)
	* [访问实例属性x](#访问实例属性x)
* [类属性访问](#类属性访问)
	* [访问类属性wing](#访问类属性wing)
	* [访问类属性x](#访问类属性x)
* [不同](#不同)
	* [有slots](#有slots)
		* [在创建 `class A` 时属性是如何初始化的 ?](#在创建-class-A-时属性是如何初始化的-?)
		* [在创建 `instance a` 时属性是如何初始化的 ?](#在创建instance-a时属性是如何初始化的-?)
		* [MRO中的属性搜索过程?](#MRO中的属性搜索过程?)
	* [没有slots](#没有slots)
		* [在创建 `class A` 时属性是如何初始化的 ?](#在创建-class-A-时属性是如何初始化的-?)
		* [在创建 `instance a` 时属性是如何初始化的 ?](#在创建instance-a时属性是如何初始化的-?)
		* [MRO中的属性搜索过程?](#MRO中的属性搜索过程?)
	* [内存消耗测试](#内存消耗测试)
* [相关阅读](#相关阅读)

# 相关位置文件

* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Objects/object.c
* cpython/Include/cpython/object.h
* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h
* cpython/Python/structmember.c
* cpython/Include/structmember.h


# slot

阅读之前需要了解的知识

* python 的属性访问行为 (在 [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md) 中有详细描述)
* python 的 descriptor protocol (在 [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md) 中有提到)
* python MRO (在 [type](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type_cn.md) 中有介绍)

# 示例

    class A(object):
        __slots__ = ["wing", "leg"]

        x = 3


访问实例 `a` 的 `wing` 和 `x` 这两个属性的时候有什么不同 ?

访问类型 `A` 的 `wing` 和 `x` 这两个属性的时候有什么不同 ?

# 实例属性访问

## 访问实例属性wing

### 设置了值之前

	>>> a = A()
    >>> a.wing
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
    AttributeError: wing

根据 [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md) 中描述的属性访问过程

我们可以画出访问 `a.wing` 的时候的粗略的流程

![instance_desc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_desc.png)

根据 **descriptor protocol** 第一步找到的对象为 `descr`, 类型为 `member_descriptor`, 如果你执行

	repr(descr)
    descr: <member 'wing' of 'A' objects>

`descr` 有一个元素名为 `d_member`, 这个元素存储了属性的名称, 属性类型和属性存储对应的位置偏移等信息

通过 `d_member` 中的信息, 你可以非常快速的定位到对应的属性的位置

在当前的示例中, 如果实例 `a` 的开始地址为 `0x00`, 那么加上这个位置偏移之后的地址为 `0x18`, 把 `0x18` 强制转换为一个 python 对象的类型, 就是你需要的属性

	/* Include/structmember.h */
    #define T_OBJECT_EX 16  /* 和 T_OBJECT 作用相同, 但是当值为空指针时会抛出 AttributeError, 而不是返回 None */

	/* Python/structmember.c */
    addr += l->offset;
    switch (l->type) {
    	/* ... */
        case T_OBJECT_EX:
            v = *(PyObject **)addr;
            /* 因为示例中的实例 a 的 wing 属性从来没有设置过其他值, 会进入到抛出 AttributeError 的语句中 */
            if (v == NULL)
                PyErr_SetString(PyExc_AttributeError, l->name);
            Py_XINCREF(v);
            break;
        /* ... */
    }

![offset](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/offset.png)

### 设置了值之后

	>>> a = A()
    >>> a.wing = "wingA"
    >>> a.wing
    'wingA'

过程和上面的过程是一样的, 但是因为实例 `a` 的 wing 属性已经设置过了一个值, `AttributeError` 不会被抛出

## 访问实例属性x

	>>> a.x
	>>> 3

`descr` 的类型为 `int`, 它并不是一个 data descriptor(没有定义 `__get__` 或者 `__set__` 方法), 所以这个 `descr` 对象会被直接返回

![instance_normal](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_normal.png)

如果 `A` 定义了 `__slots__`, 你就不能在在实例 `a` 中定义任何其他的属性, 我们后面会看一下为什么会这样

	>>> a.not_exist = 100
	Traceback (most recent call last):
	File "<input>", line 1, in <module>
	AttributeError: 'A' object has no attribute 'not_exist'

# 类属性访问

## 访问类属性wing

	>>> A.wing
	<member 'wing' of 'A' objects>
    >>> type(A.wing)
    <class 'member_descriptor'>

访问 `A.wing` 和访问 `a.wing` 的过程大致相同

![type_desc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/type_desc.png)

## 访问类属性x

	>>> A.x
	3

访问 `A.x` 和访问 `a.x` 的过程大致相同的

![type_normal](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/type_normal.png)

# 不同

## 有slots

### 在创建`class A`时属性是如何初始化的 ?

我们在 [type->class 的创建](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type_cn.md#class-的创建) 这篇文章里面已经学习了类型的创建过程

`type` 对象在 C 语言的定义中是一个比较多字段的结构体, 接下来的图片示例只会展示当前文章主题相关的字段

`__slots__` 中定义的属性名称在类型A的创建过程中会被排序, 并转换为一个元组对象, 之后存储在类型A的 `ht_slots` 字段中

当前定义的 `__slots__` 中的两个属性会在新创建的类型A的尾部中预先分配好位置, 并以 `PyMemberDef` 指针的形式按照 `ht_slots` 中的顺序存储在其中

对于属性 `x` 并无特殊处理, 保存在 `tp_dict` 字段指向的字典中

并且 `tp_dict` 字段指向的字典中没有 `"__dict__"` 这个 key (只要定义了 `__slots__` 的类型都不会有)

![type_create](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/type_create.png)

### 在创建`instance a`时属性是如何初始化的 ?

`__slots__` 中需要存储的属性是在实例创建过程中预先分配的

![instance_create](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_create.png)

### MRO中的属性搜索过程 ?

遍历 MRO 中的每一个类型, 如果这个被搜索的名称在这个类型的 `tp_dict` 中, 返回 `tp_dict[name]`

	/* cpython/Objects/typeobject.c */
    /* for the instance a, if we access a.wing
       type: <class '__main__.A'>
       mro: (<class '__main__.A'>, <class 'object'>)
       name: 'wing'
    */
	mro = type->tp_mro;
    n = PyTuple_GET_SIZE(mro);
    for (i = 0; i < n; i++) {
        base = PyTuple_GET_ITEM(mro, i);
        dict = ((PyTypeObject *)base)->tp_dict;
        // in python representation: res = dict[name]
        res = _PyDict_GetItem_KnownHash(dict, name, hash);
        if (res != NULL)
            break;
        if (PyErr_Occurred()) {
            *error = -1;
            goto done;
        }
    }

比如我们尝试获取属性 `wing` 时

	>>> type(A.wing)
	<class 'member_descriptor'>
	>>> type(a).__mro__
	(<class '__main__.A'>, <class 'object'>)
    >>> print(a.wing)
    wingA

下面的伪代码翻译了 C 语言中搜索过程

	res = None
    for each_type in type(a).__mro__:
    	if "wing" in each_type.__dict__:
        	res = each_type.__dict__["wing"]
            break
    # 接下来是另一篇文章提到的属性访问过程
    ...
    # 这是属性访问过程的一个情况, 也是访问当前属性时会发生的情况
    if res is a data_descriptor:
        # res 在这里是 A.wing, 它的类型是 member_descriptor
        # 它存储了这个属性的位置偏移等信息, 实例可以根据这个上面的信息快速的获取到需要的对象
        # member_descriptor.__get__ 会找到 a + offset 的地址, 并把这个地址强制转换为 PyObject *, 并返回给调用着
        return res.__get__(a, type(a))
    ...

![access_slot_attribute](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_slot_attribute.png)

![access_slot_attribute2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_slot_attribute2.png)

如果我们尝试访问或者设置一个不存在的属性

	>>> a.not_exist = 33
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
    AttributeError: 'A' object has no attribute 'not_exist'

根据 [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md) 中提到的 **descriptor protocol** 过程, 我们可以同样写出下面的伪代码

	res = None
	for each_type in type(a).__mro__:
   		if "not_exist" in each_type.__dict__:
        	res = each_type.__dict__["not_exist"]
        	break
    if res is None:
        # 尝试在 a.__dict__ 中查找 "not_exist"
        if not hasattr(a, "__dict__") or "not_exist" not in a.__dict__:
        	# 运行到这里
        	raise AttributeError
        return a.__dict__["not_exist"]

当定义了 `__slots__` 时, `type(a)` 中的 `tp_dictoffset` 值为 0, 表示实例 `a` 并不存在 `__dict__` 属性, 也就是说没有存储其他任何属性的位置, 上面进入的搜索分支会识别这种情况并报错

所以会抛出 `AttributeError`

![access_slot_not_exist_attribute](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_slot_not_exist_attribute.png)


## 没有slots

    class A(object):

        x = 3
        wing = "wingA"
        leg = "legA"

### 在创建`class A`时属性是如何初始化的 ?

`tp_dict` 指向的字典对象现在有一个名为 `__dict__` 的 key

![type_create_no_slot](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/type_create_no_slot.png)

### 在创建`instance a`时属性是如何初始化的 ?

![instance_create_no_slot](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_create_no_slot.png)

### MRO中的属性搜索过程 ?

搜索过程和 [有slots](#有slots) 的搜索过程类似

![access_no_slot_attribute](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_no_slot_attribute.png)

如果我们尝试访问或者设置一个不存在的属性

	>>> a.not_exist = 33
	>>> print(a.not_exist)

根据 [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md) 中提到的 descriptor protocol, 我们可以同样写出下面的伪代码

	res = None
	for each_type in type(a).__mro__:
   		if "not_exist" in each_type.__dict__:
        	res = each_type.__dict__["not_exist"]
        	break
    if res is None:
        # 尝试在 a.__dict__ 中查找 "not_exist"
        if not hasattr(a, "__dict__") or "not_exist" not in a.__dict__:
        	raise AttributeError
        # 运行到这里
        return a.__dict__["not_exist"]

这一次没有定义 `__slots__`, `type(a)` 中的 `tp_dictoffset` 值为 16, 表示实例 `a` 拥有 `__dict__` 属性, 可以存储任意其他的属性名称, 这个 `__dict__` 对象的地址为 `(char *)a + 16`

所以属性名称可以存储在 `a.__dict__` 中

![access_no_slot_not_exist_attribute](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_no_slot_not_exist_attribute.png)

![access_no_slot_not_exist_attribute2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/access_no_slot_not_exist_attribute2.png)

## 内存消耗测试

有 `__slots__`

	./ipython
	>>> import ipython_memory_usage.ipython_memory_usage as imu
    >>> imu.start_watching_memory()
    In [2] used 0.1367 MiB RAM in 3.59s, peaked 0.00 MiB above current, total RAM usage 41.16 MiB
    class MyClass(object):
        __slots__ = ['name', 'identifier']
        def __init__(self, name, identifier):
                self.name = name
                self.identifier = identifier
    num = 1024*256
    x = [MyClass(1,1) for i in range(num)]

    used 27.5508 MiB RAM in 0.28s, peaked 0.00 MiB above current, total RAM usage 69.18 MiB

没有 `__slots__`

    ./ipython
	>>> import ipython_memory_usage.ipython_memory_usage as imu
    >>> imu.start_watching_memory()
    In [2] used 0.1367 MiB RAM in 3.59s, peaked 0.00 MiB above current, total RAM usage 41.16 MiB

    class MyClass(object):
            def __init__(self, name, identifier):
                    self.name = name
                    self.identifier = identifier
    num = 1024*256
    x = [MyClass(1,1) for i in range(num)]

    used 56.0234 MiB RAM in 0.34s, peaked 0.00 MiB above current, total RAM usage 97.63 MiB

没有 `__slots__` 的情况下内存消耗几乎是有 `__slots__` 的情况下的两倍, 主要原因是有 `__slots__` 的时候属性的空间是在实例创建时一次性预分配好的, 存储的是指向 python 元素的指针, 每个 指针占用 8 字节的空间, 而没有 `__slots__` 的时候需要创建一个额外的 [dict 对象](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_cn.md) 用来存储, 新增, 删除属性, 虽然时间复杂度类似, 但是 [dict 对象](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_cn.md) 本身字典结构存储需要空间, 并且在当前版本下即使创建了空的字典, 本身也会预分配 8 个对象的空间, 即使是一个空字典也至少是需要一打指针的空间来存储, 这些都是额外开销

# 相关阅读
* [`__slots__`magic](http://book.pythontips.com/en/latest/__slots__magic.html)
* [A quick dive into Python’s `__slots__`](https://blog.usejournal.com/a-quick-dive-into-pythons-slots-72cdc2d334e?gi=26f20645d6c9)