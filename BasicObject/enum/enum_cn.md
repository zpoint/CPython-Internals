# enum

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
	* [normal](#normal)
	* [en_longindex](#en_longindex)

#### 相关位置文件

* cpython/Objects/enumobject.c
* cpython/Include/enumobject.h
* cpython/Objects/clinic/enumobject.c.h

#### 内存构造

**enumerate** 是一个类型, **enumerate** 的实例是一个可迭代对象, 你可以在迭代的过程中同时获得这个迭代的对象和一个计数器

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/layout.png)

#### 示例

##### normal

    def gen():
        yield "I"
        yield "am"
        yield "handsome"

    e = enumerate(gen())

    >>> type(e)
    <class 'enumerate'>

在迭代对象 **e** 之前, 它的 **en_index** 字段为 0, **en_sit** 指向了真正的 **generator** 对象, **en_result** 指向了一个有两个空值的 **tuple**

我们后面会提到 **en_longindex** 的作用

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example0.png)

    >>> t1 = next(e)
    >>> t1
    (0, 'I')
    >>> id(t1)
    4469348888

现在 **en_index** 字段变成了 1, **en_result** 里面指向的元组对象为最近一次返回的元组对象, 元组里面的两个元素都改变了, 但是 **en_result** 里的地址没有变化, 没有变化的原因不是 [tuple 缓冲池](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md#free-list) 机制的原因

而是和 **enumerate** 的迭代函数使用的技巧有关

    static PyObject *
    enum_next(enumobject *en)
    {
    	/* omit */
        PyObject *result = en->en_result;
		/* omit */
        if (result->ob_refcnt == 1) {
        	/*
            tuple 对象的引用计数器为 is 1
           说明当前唯一引用这个 tuple 对象的就是这个 enumerate 实例本身
           既然这个 tuple 对象的两个旧元素已经不需要了
           我们可以把这两个元素设置为新的元素, 并返回这个 tuple 对象
            */
            Py_INCREF(result);
            old_index = PyTuple_GET_ITEM(result, 0);
            old_item = PyTuple_GET_ITEM(result, 1);
            PyTuple_SET_ITEM(result, 0, next_index);
            PyTuple_SET_ITEM(result, 1, next_item);
            Py_DECREF(old_index);
            Py_DECREF(old_item);
            return result;
        }
        /*
        到达这里, 这个 tuple 对象的引用计数器不为 1, 处理自己本身还有其他的变量在使用它
        我们不能重置这个 tuple 里面的元素
        只能创建新的返回给调用者
        */
        result = PyTuple_New(2);
        if (result == NULL) {
            Py_DECREF(next_index);
            Py_DECREF(next_item);
            return NULL;
        }
        PyTuple_SET_ITEM(result, 0, next_index);
        PyTuple_SET_ITEM(result, 1, next_item);
        return result;
    }

很明显了, 因为旧的 **tuple** `tuple object -> (None, None)` 对象唯一的引用来自当前的 **enumerate** 对象, **enum_next** 会把这个 **tuple** 对象的第 0 个元素变为 0, 第 1 个元素变为 'I'， 之后把这个旧 tuple 返回

所以 **en_result** 指向的地址未发生改变, 并且 **en_result** 指向的对象为这次迭代返回的对象

![example1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example1.png)

这个 **tuple** 对象 `(0, 'I') # id(4469348888)` 现在的引用计数器变为 2 了, 一个来自 **enumerate** 实例的引用, 还有一个来自变量名称 t1, **enum_next** 这次会进入下面的分支, 创建一个新的 **tuple** 对象并返回而不是重置那个旧的 **tuple** 对象

**en_index** 中的数值增加了, **en_result** 仍然指向这个旧的 **tuple** 对象`(0, 'I') # id(4469348888)`

	>>> next(e)
	(1, 'am')

![example2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example2.png)

`del t1` 语句执行之后, 这个 tuple 对象 `(0, 'I') # id(4469348888)` 的引用计数器变回了 1, 此时 **enum_next** 会像上面一样重置这个 **tuple** 对象, **en_result** 仍然指向这个对象, 并且这次返回的为 **en_result** 指向的对象

	>>> del t1 # decrement the reference count of the object referenced by t1
	>>> next(e)
	(2, 'handsome')

![example3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example3.png)

结束标记是被 **en_sit** 里面存储的对象所存储的, **enumerate** 本身不存储结束标记等信息

    >>> next(e)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![example3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example3.png)

##### en_longindex

通常情况下, 计数器的值是存储在 **en_index** 里面的, **en_index** 的类型是 **Py_ssize_t**, 我们来看下 **Py_ssize_t** 的定义

    #ifdef HAVE_SSIZE_T
    typedef ssize_t         Py_ssize_t;
    #elif SIZEOF_VOID_P == SIZEOF_SIZE_T
    typedef Py_intptr_t     Py_ssize_t;
    #else
    #   error "Python needs a typedef for Py_ssize_t in pyport.h."
    #endif

大部分情况下他是一个 **ssize_t**, 32位操作系统下为 int, 64位下为 **long int**

在我的机器上他是 **long int**

如果这个计数器的值大到这 64个 bit 都装不下呢?

	e = enumerate(gen(), 1 << 62)

这个时候 **en_index** 是可以放得下这个值的

![longindex0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/longindex0.png)

**en_index** 能表示的最大的值为 ((1 << 63) - 1) (PY_SSIZE_T_MAX)

现在实际上的计数器的值已经比 PY_SSIZE_T_MAX 还要大了, 此时 **en_longindex** 会被用来存储真正的计数器

**en_longindex** 指向的是一个 [PyLongObject(python 类型 int)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long_cn.md),  对象, 可以表示任意长度的整数大小

	>>> e = enumerate(gen(), (1 << 63) + 100)

![longindex1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/longindex1.png)

