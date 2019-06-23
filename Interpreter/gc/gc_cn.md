# gc

* 长文, 预计阅读时间在十五分钟以上

# 目录

* [相关位置文件](#相关位置文件)
* [介绍](#介绍)
	* [引用计数器机制](#引用计数器机制)
		* [什么时候会触发该机制](#什么时候会触发该机制)
		* [引用循环的问题](#引用循环的问题)
			* [example1](#example1)
			* [example2](#example2)
	* [分代回收机制](#分代回收机制)
		* [track](#track)
		* [generational](#generational)
		* [update_refs](#update_refs)
		* [subtract_refs](#subtract_refs)
		* [move_unreachable](#move_unreachable)
		* [finalizer](#finalizer)
		* [threshold](#threshold)
		* [什么时候会触发分代回收](#什么时候会触发分代回收)
* [总结](#总结)
* [更多资料](#更多资料)

# 相关位置文件

* cpython/Include/object.h
* cpython/Modules/gcmodule.c
* cpython/Include/internal/pycore_pymem.h

# 介绍

CPython 中的垃圾回收机制包含了两个部分

* **引用计数器机制** (大部分在 `Include/object.h` 中定义)
* **分代回收机制** (大部分在 `Modules/gcmodule.c` 中定义)

实际上 **分代回收机制** 是 **分代回收** 和 **标记清除** 等一系列操作的结合, 这里我为了命名方便就叫做 **分代回收机制**

## 引用计数器机制

如 [维基百科](https://zh.wikipedia.org/wiki/%E5%BC%95%E7%94%A8%E8%AE%A1%E6%95%B0) 所说

> 在计算机科学中, 引用计数是计算机编程语言中的一种内存管理技术，是指将资源（可以是对象、内存或磁盘空间等等）的被引用次数保存起来，当被引用次数变为零时就将其释放的过程。使用引用计数技术可以实现自动资源管理的目的。同时引用计数还可以指使用引用计数技术回收未使用资源的垃圾回收算法。

> 当创建一个对象的实例并在堆上申请内存时，对象的引用计数就为1，在其他对象中需要持有这个对象时，就需要把该对象的引用计数加1，需要释放一个对象时，就将该对象的引用计数减1，直至对象的引用计数为0，对象的内存会被立刻释放。

在 python 中

	s = "hello world"

    >>> id(s)
    4346803024
    >>> sys.getrefcount(s)
    2 # 一个来自变量 s, 一个来自 sys.getrefcount 的参数

![refcount1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount1.png)

	>>> s2 = s
	>>> id(s2)
	4321629008 # 和 id(s) 的值一样
    >>> sys.getrefcount(s)
    3 # 多出来的一个来自 s2

![refcount2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount2.png)


我们来 dis 一个脚本

	$ cat test.py
    s = []
    s2 = s
    del s
    $ ./python.exe -m dis test.py

输出如下

    1         0 BUILD_LIST               0
              2 STORE_NAME               0 (s)

    2         4 LOAD_NAME                0 (s)
              6 STORE_NAME               1 (s2)

    3         8 DELETE_NAME              0 (s)
             10 LOAD_CONST               0 (None)
             12 RETURN_VALUE

代码第一行 `s = []`

`0 BUILD_LIST               0` 向 heap 申请了部分空间, 创建了一个空白的新 list 对象, 把这个空白列表的引用计数设置为 1, 并且压入 stack 中

`2 STORE_NAME               0`

    case TARGET(STORE_NAME): {
        /* name 是一个 str 对象, 值为 's'
           ns 是 local namespace
           v 是前一个字节码创建的 list 对象
        */
        PyObject *name = GETITEM(names, oparg);
        PyObject *v = POP();
        PyObject *ns = f->f_locals;
        int err;
        if (ns == NULL) {
            PyErr_Format(PyExc_SystemError,
                         "no locals found when storing %R", name);
            Py_DECREF(v);
            goto error;
        }
        if (PyDict_CheckExact(ns))
        	/* 在这个位置, v 的引用计数 为 1
               PyDict_SetItem 会把 's' 作为键加到 local namespace 中, 值为对象 v
               ns 类型为 字典对象, 这一操作会同时把 's' 和 v 的引用计数都增加 1
           */
            err = PyDict_SetItem(ns, name, v);
            /* 做完上面的操作, v 的引用计数变为了 2 */
        else
            err = PyObject_SetItem(ns, name, v);
        Py_DECREF(v);
        /* Py_DECREF 之后, v 的引用计数变为了 1 */
        if (err != 0)
            goto error;
        DISPATCH();
    }

在执行第二行 `s2 = s` 之前, 新创建的 list 对象的引用计数为 1

`4 LOAD_NAME                0` 会取出存在 local namespace 键为 's' 的对象(就是前面新创建的列表对象), 把它的引用计数加 1, 并推入 stack 中

到这一步, 新创建的 list 对象的引用计数为 2(一个来自字典对象 local namespace 中的 's', 一个来自 stack)

`6 STORE_NAME               1` 再次执行上面字节码对应的代码, name 这次变为了 's2', 在这个字节码之后, list 对象的引用计数变为了 2

`8 DELETE_NAME              0 (s)`

    case TARGET(DELETE_NAME): {
    	/* name 这里为 's'
           ns 为 the local namespace
        */
        PyObject *name = GETITEM(names, oparg);
        PyObject *ns = f->f_locals;
        int err;
        if (ns == NULL) {
            PyErr_Format(PyExc_SystemError,
                         "no locals when deleting %R", name);
            goto error;
        }
        /* 到这里, list 对象的引用计数为 2
           下面的操作会找到键 's' 对应的位置, 把 indices 设置为 DKIX_DUMMY,
           entries 中的 key 和 value 位置都置为空指针, 并把 key 和 value 本身对象引用计数减1
        */
        err = PyObject_DelItem(ns, name);
        /* 到了这里, list 对象的引用计数变为了 1 */
        if (err != 0) {
            format_exc_check_arg(PyExc_NameError,
                                 NAME_ERROR_MSG,
                                 name);
            goto error;
        }
        DISPATCH();
    }

### 什么时候会触发该机制

如果一个对象的引用计数变为了 0, 会直接进入释放空间的流程

	/* cpython/Include/object.h */
    static inline void _Py_DECREF(const char *filename, int lineno,
                                  PyObject *op)
    {
        _Py_DEC_REFTOTAL;
        if (--op->ob_refcnt != 0) {
    #ifdef Py_REF_DEBUG
            if (op->ob_refcnt < 0) {
                _Py_NegativeRefcount(filename, lineno, op);
            }
    #endif
        }
        else {
        	/* // _Py_Dealloc 会找到对应类型的 descructor, 并且调用这个 descructor
            destructor dealloc = Py_TYPE(op)->tp_dealloc;
            (*dealloc)(op);
            */
            _Py_Dealloc(op);
        }
    }

### 引用循环的问题

有一些情况是引用计数器没办法处理的

### example1

    class A:
        pass

    >>> a1 = A()
    >>> a2 = A()
    >>> a1.other = a2
    >>> a2.other = a1

**a1** 和 **a2** 的引用计数都为 2

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each1.png)

    >>> del a1
    >>> del a2

现在, 来自 local namespace 的引用被清除了, 但是他们自身都有一个来自对方的引用, 按照上面的过程 **a1**/**a2** 的引用计数是没办法变为 0 的

![ref_cycle2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each2.png)

### example2

	>>> a = list()
	>>> a.append(a)
	>>> a
	[[...]]

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle1.png)

	>>> del a

来自 local namespace 的引用被清除了, **a** 的引用计数会变为 1, 也没有办法变为 0 进入释放流程


![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle2.png)

## 分代回收机制

如果只有一个 **引用计数器机制** 在工作, 当上面引用循环的对象变得多起来, 解释器进程就会发生内存泄漏

**分代回收机制** 是另一部分用来处理上面无法处理到的引用循环的那部分对象

实际上 **分代回收机制** 是 **分代回收** 和 **标记清除** 等一系列操作的结合, 这里我为了命名方便就叫做 **分代回收机制**

    class A:
        pass

    >>> a1 = A()
    >>> a2 = A()
    >>> a1.other = a2
    >>> a2.other = a1

    >>> b = list()
    >>> b.append(b)

### track

肯定存在一个办法能追踪到所有从 heap 中新分配的对象

**generation0** 是一个指针, 指向一个链表的头部, 链表中的每一个元素都包含了两部分, 上半部分为 **PyGC_Head**, 下半部分为 **PyObject** (这两部分是连续的内存)

**PyGC_Head** 包括了一个 **_gc_next** 指针(指向链表的下一个元素), 和一个 **_gc_prev** 指针(指向链表的上一个元素) (垃圾回收这部分, **_gc_prev** 的某些bit被用来作为标记清除, 并不是单纯的链表的链接)

**PyObject** 则是所有 python 对象都必须包含的基础部分

![track](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/track.png)

当你创建一个 python 对象比如 `a = list()` 时, 类型为 **list**, 变量名为 **a** 的对象会被添加到 **generation0** 的尾部, 所以 **generation0** 可以追踪到所有通过解释器从 heap 空间分配的对象

### generational

要提高垃圾回收的速度, 要么就改进垃圾回收算法, 加快每次回收的效率, 要么就减小每次回收需要处理的对象

如果每个新增的对象都添加到链表尾部, 假以时日, 这会是一个非常长的链表

对于一些服务型应用, 肯定存在一些长时间存活的对象, 重复的对这些对象进行垃圾回收会浪费性能

分代回收的方法是针对减小每次需要回收的对象所提出的

CPython 总共使用了 3 代, 新创建的对象都会被存储到第一代中(**generation0**), 当一个对象在一次垃圾回收后存活了, 这个对象会被移动到相近的下一代中

代越年轻, 里面的对象也就越年轻, 年轻的代也会比年长的代更频繁的进行垃圾回收

当准备回收一个代的时候, 所有比这个代年轻的代都会被合并, 之后再回收

![generation](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation.png)

### update_refs

我们来跑一个垃圾回收的示例看看

	# 假设 a1, a2, b 都在 generation0 中存活了一次, 并移动到了 generation1
    >>> c = list()
    >>> d1 = A()
    >>> d2 = A()
    >>> d1.other2 = d2
    >>> d2.other2 = d1

    >>> del a1
    >>> del a2
    >>> del b
    >>> del d1
	>>> gc.collect(1) # 假设回收的是 generation1

![update_ref1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref1.png)

第一步是把所有比 **generation1** 低的代和当前代合并, 把这个合并后的代叫做 **young**, 把比当前代年长一代的那一代叫做 **old**

![young_old](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/young_old.png)

**update_refs** 会把所有 **young** 中对象的引用计数拷贝出来, 复制到对象上面的 **_gc_prev** 位置中,
**_gc_prev** 的最右边两个 bit 被预留出来作为其他标记用, 所以拷贝的引用计数会左移两位后存储在 **_gc_prev** 上

我下图表示的 `1 / 0x06` 意思是复制出的引用计数值为 1, 但是 **_gc_prev** 上实际存储的值为 `0x06`

![update_ref2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref2.png)

### subtract_refs

**subtract_refs** 会遍历 **young** 中的所有对象, 对于每个对象, 检查当前对象身上所引用到的所有其他对象, 这些其他对象如果也在 **young** 中, 并且是可回收的对象, 把这个对象复制到 **_gc_prev** 的引用计数部分减1 (用 **tp_traverse** 遍历, 遍历回调函数为 **visit_decref**)

这一步的目的是消除同一个代中对象间的引用, 消除完后, 剩下的引用计数都是来自代外的对象的引用计数

![subtract_refs](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/subtract_refs.png)

### move_unreachable

**move_unreachable** 会创建一个新的叫做 **unreachable** 的链表, 遍历当前名为 **young** 的 **generation**, 把那些上一步处理过的引用计数小于等于 0 的对象移动到 **unreachable** 中

如果当前对象复制出的引用计数  > 0, 那么对于当前对象所引用到的, 所有其他的在用一个 **generation** 中的对象, 如果这个其他对象的复制引用计数 <= 0, 直接把它设置成 1, 如果这个对象在 **unreachable** 中, 把它移回 **generation** 中

我们来看一个示例

* 注意, 以下的部分, **generation** 中存储的是实际的 PyObject 对象, 变量比如 `a1`, `a2` 是对实际对象的一个引用, 下面为了方便说明进行了混用

**generation** 中的第一个对象是 local **namespace**, 因为 **namespace** 对象的引用数 > 0, 它不能被回收, 所有 **namespace** 对象所引用到的对象也肯定是不能被回收的, 所以 `c` and `d2` 的 **_gc_prev** 值被设置为 `0x06`

![move_unreachable1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable1.png)

第二个对象是 `a1`, 因为 `a1` 的复制引用计数值 <= 0, 所以它被移到了 **unreachable** 中

![move_unreachable2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable2.png)

`a2` 也一样

![move_unreachable3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable3.png)

`b` 的复制引用计数也 <= 0

![move_unreachable4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable4.png)

现在轮到了 `c`

因为 `c` 的复制引用计数 > 0, 它会继续留在 **generation** 中, 并且 **_gc_prev** 会被重置(上面的 collecting 的 bit flag 会被清除)

![move_unreachable5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable5.png)

因为 `d1` 的复制引用计数 <= 0, 它被移动到 **unreachable**

![move_unreachable6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable6.png)

`d2` 的复制引用计数 > 0, `d2` 身上还有到 `d1` 的引用, 并且 `d1` 现在在 **unreachable** 中

![move_unreachable7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable7.png)

`d1` 的复制引用计数会被重置为 1, 并且移到 **generation** 这个链表的尾部

![move_unreachable8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable8.png)

下一个对象是 `d1`, 它的复制引用计数 > 0, 重置它的 **_gc_prev** 指针并

我们到达了 **generation** 的尾部, 现在这一刻, **unreachable** 中的所有对象可以进行真正的回收流程

剩下的所有在 **generation** 中的对象是存活的, 活过一轮垃圾回收的对象会被移动到更高一代中去

![move_unreachable9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable9.png)

### finalizer

如果一个需要被垃圾回收的对象定义了自己的 `__del__` 怎么办呢? 毕竟有可能在 `__del__` 中增加了新的引用

在 python3.4 之前, 这部分对象即使被移动到了 **unreachable**, 也是不会被释放回 heap 空间中的

在 python3.4 后, [pep-0442](https://legacy.python.org/dev/peps/pep-0442/) 解决了这个问题

我们来看个示例

    class A:
        pass


    class B(list):
        def __del__(self):
            a3.append(self)
            print("del of B")


    a1 = A()
    a2 = A()
    a1.other = a2
    a2.other = a1


    a3 = list()
    b = B()
    b.append(b)
    del a1
    del a2
    del b
    gc.collect()

上面的代码在 **move_unreachable** 之后如下所示

![finalize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize1.png)

step1, 所有 **unreachable** 中定义了自己的 finalizer 的对象, 这些个对象的 `__del__` 方法都会被调用

在 `__del__` 调用后

![finalize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize2.png)

step2, 在 **unreachable** 里面做一次 **update_refs**

注意, 这里 `b` **_gc_prev** 字段的第一个 bit 被置为 1 了, 表示它的 finalizer 已经被处理过了

![finalize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize3.png)

step3, 在 **unreachable** 里面做一次 **subtract_refs**

这一步是为了消除在 `__del__` 中, **unreachable** 中对象之间创建的互相引用

之后遍历每个对象, 检查是否有复制引用计数 > 0 的对象, 如果有, 说明有某对象的 `__del__` 方法使得 **unreachable** 以外的对象增加了对其自身的引用, 如果有的话, 遍历完后进入 step4

如果没有, 则回收 **unreachable** 中的所有对象

![finalize4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize4.png)

stp4, 把所有的 **unreachable** 中的对象移动到 **old** 代中

在 step1 中, **unreachable** 中定义了 `__del__` 的对象的对应的 `__del__` 都会被调用, 并且所有的 **unreachable** 中的对象都会在当前这轮垃圾回收中存活

![finalize5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize5.png)

在下一轮垃圾回收中, 对象 `a1` 和 `a2` 会被回收, 但是对象 `b` 的引用计数在下一轮的 **subtract_refs** 之后仍然是大于 0 的, 所以它不会被移到 **unreachable** 中

如果 `__del__` 被调用过, **_gc_prev** 上的第一个 bit flag 会被设置, 所以一个对象的 `__del__` 仅会被调用一次

### threshold

CPython 中一共有 3 代, 对应了 3 个 **threshold**, 每一代对应一个 **threshold**

在进行回收之前, CPython 会从最老年的代到最年轻的代进行检测, 如果当前代中对象数量超过了 **threshold**, 垃圾回收就从这一代开始

    >>> gc.get_threshold()
    (700, 10, 10) # 默认值

也可以手动进行设置

    >>> gc.set_threshold(500)
    >>> gc.set_threshold(100, 20)

### 什么时候会触发分代回收

一个方式是直接调用 `gc.collect()`, 不传参数的情况下直接从最老年代开始回收

	>>> imporr gc
	>>> gc.collect()

另一个方式是让解释器自己进行触发, 当你从 heap 申请空间创建一个新对象时, **generation0** 的数量是否超过 **threashold**, 如果超过, 进行垃圾回收, 如果没超过则创建成功, 返回

![generation_trigger1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger1.png)

`collect` 流程会从最老年代向最年轻代进行检查

首先检查 **generation2**, **generation2** 的 count 比对应的 **threashold** 小

![generation_trigger2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger2.png)

之后检查 **generation1**, **generation1** 的 count 比对应的 **threashold** 大, 那么从 **generation1** 开始回收

![generation_trigger3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger3.png)

回收的时候会把所有比当前代年轻的代合并后, 再进行上面描述的回收步骤

![generation_trigger4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger4.png)

如果 gc 回收的是最年长的一代, 回收结束之前会把所有的 **free_list** 也一并释放, 如果你读过主页其他对象的文章比如 [list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_cn.md) 或 [tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md), 上面有 **free_list** 相关的作用

# 总结

因为 CPython 使用的垃圾回收算法并非并发算法, 一个全局锁的存在是有必要的, 比如 [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_cn.md), 它可以在设置 track 的时候, 引用计数变更的时候, 进行分代回收, 标记清除等过程中保护这些变量

其他语言会实现会实现可以并发的垃圾回收算法, 比如 [Java-g1](http://idiotsky.top/2018/07/28/java-g1/) 可以配置 **Young GC** 或者 **Mix GC**(涉及到支持全局并发标记的三色标记算法)

# 更多资料

* [Garbage collection in Python: things you need to know](https://rushter.com/blog/python-garbage-collector/)
* [the garbage collector](https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/)