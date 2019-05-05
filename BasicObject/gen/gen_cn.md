# gen

### 目录

* [相关位置文件](#相关位置文件)
* [generator](#generator)
	* [内存构造](#内存构造-generator)
	* [示例 generator](#示例-generator)
* [coroutine](#coroutine)
	* [内存构造](#内存构造-coroutine)
	* [示例 coroutine](#示例-coroutine)
* [async generator](#async-generator)
	* [内存构造](#内存构造-async-generator)
	* [示例 async generator](#示例-async-generator)
	* [free list](#free-list)

### 相关位置文件
* cpython/Objects/genobject.c
* cpython/Include/genobject.h

### generator

#### 内存构造 generator

**generator**, **coroutine** 和 **async generator** 共享一大部分的定义

    #define _PyGenObject_HEAD(prefix)                                           \
        PyObject_HEAD                                                           \
        /* Note: gi_frame can be NULL if the generator is "finished" */         \
        struct _frame *prefix##_frame;                                          \
        /* True if generator is being executed. */                              \
        char prefix##_running;                                                  \
        /* The code object backing the generator */                             \
        PyObject *prefix##_code;                                                \
        /* List of weak reference. */                                           \
        PyObject *prefix##_weakreflist;                                         \
        /* Name of the generator. */                                            \
        PyObject *prefix##_name;                                                \
        /* Qualified name of the generator. */                                  \
        PyObject *prefix##_qualname;                                            \
        _PyErr_StackItem prefix##_exc_state;

**generator** 对象实际上的定义仅不到4行代码

    typedef struct {
        /* The gi_ prefix is intended to remind of generator-iterator. */
        _PyGenObject_HEAD(gi)
    } PyGenObject;

我们可以把他扩展一下

    typedef struct {
        struct _frame *gi_frame;
        char gi_running;
        PyObject *gi_code;
        PyObject *gi_weakreflist;
        PyObject *gi_name;
        PyObject *gi_qualname;
        _PyErr_StackItem gi_exc_state;
    } PyGenObject;

根据扩展后的代码可以直观的画出内存构造

![layout_gen](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_gen.png)

#### 示例 generator

我们来定义一个 **generator** 并一步一步的迭代他

    def fib(n):
        t = 0
        i = 1
        j = 1
        r = 0
        result = None
        while t <= n:
            print("result", repr(result))
            if t < 2:
                result = yield i
            else:
                r = i + j
                result = yield r
                i = j
                j = r
            t += 1
        try:
            1 / 0
        except ZeroDivisionError:
            r = yield "ZeroDivisionError"
            print(repr(r))
            try:
                import empty
            except ModuleNotFoundError:
                result = yield "ModuleNotFoundError"
                print("result", repr(result))
            finally:
                result = yield "ModuleNotFoundError finally"
                print("result", repr(result))
        raise StopIteration

	>>> f = fib(5)
    >>> type(f)
    <class 'generator'>
    >>> type(fib)
    <class 'function'>
	>>> f.gi_frame.f_lasti
	-1

我们初始化了一个新的 **generator**, **gi_frame** 字段的对象里储存的 **f_lasti** 和操作系统概念里的 program counter 有点类似, 你可以把他理解成 python 虚拟机中的 program counter, 他指向当前 **gi_code** 对象里包含的可执行代码块的位置

	>>> fib.__code__
	<code object fib at 0x1041069c0, file "<stdin>", line 1>
    >>> f.gi_code
    <code object fib at 0x1041069c0, file "<stdin>", line 1>

对象 f 里面的 **gi_code** 正是一个 **code** 对象, 这个对象包含了函数 **fib** 所需的信息

**gi_running** 为 0, 表示 **generator** 当前没有在运行中

**gi_name** 和 **gi_qualname** 都指向同一个 **unicode** 对象, **gi_exc_state** 里面的各个字段的值都为 0x00

![example_gen_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_0.png)

    >>> r = f.send(None)
    result None
    >>> f.gi_frame.f_lasti
    52
	>>> repr(r)
	'1'

对象**f**每个字段的值都没有发生变化

但是 **gi_frame** 中存储的 **f_lasti** 现在指向了 52(第一个 **yield** 出现的位置)

![example_gen_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_1.png)

迭代多一次, 由于 while 循环的原因, **f_lasti** 仍然指向同个位置

    >>> r = f.send("handsome")
    result 'handsome'
    >>> f.gi_frame.f_lasti
    52
	>>> repr(r)
	'1'

再次调用 send, **f_lasti** 此时指向第二个 **yield** 出现的位置

	>>> r = f.send("handsome2")
	result 'handsome2'
    >>> f.gi_frame.f_lasti
    68
    >>> repr(r)
    '2'

重复迭代

    >>> r = f.send("handsome3")
    result 'handsome3'
    >>> f.gi_frame.f_lasti
    68
    >>> repr(r)
    '3'
    >>> r = f.send("handsome4")
    result 'handsome4'
    >>> f.gi_frame.f_lasti
    68
    >>> repr(r)
    '5'
    >>> r = f.send("handsome5")
    result 'handsome5'
    >>> f.gi_frame.f_lasti
    68
    >>> repr(r)
    '8'

现在, 通过 break 跳出了 while 循环

**f_lasti** 指向的位置是 第一个 **except** 发生的位置, **exc_type** 指向这个异常的类型, **exc_value** 指向这个异常的实例, **exc_traceback** 指向异常追踪 traceback 对象

    >>> r = f.send("handsome6")
    >>> f.gi_frame.f_lasti
    120
    >>> repr(r)
    "'ZeroDivisionError'"

![example_gen_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_2.png)

**f_lasti** 在第二个 **except** 的位置上, **exc_type**, **exc_value**, 和 **exc_traceback** 都和异常 ModuleNotFoundError 相关联

    >>> r = f.send("handsome7")
    'handsome7'
    >>> f.gi_frame.f_lasti
    168
    >>> repr(r)
    "'ModuleNotFoundError'"

![example_gen_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_3.png)

**f_lasti** 在第一个 **finally** 的位置上, 异常 ModuleNotFoundError 已经处理完成, 异常堆的堆顶现在是一个 **ZeroDivisionError**

后续会有讲异常处理的文章 [exception](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/exception/exception_cn.md) (先留个链接)

    >>> r = f.send("handsome8")
    result 'handsome8'
    >>> f.gi_frame.f_lasti
    198
    >>> repr(r)
    "'ModuleNotFoundError finally'"

![example_gen_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_4.png)

现在 **StopIteration** 被抛出

**gi_frame** 中的 frameObject 被释放了, 变成了一个空指针, 表明这个 generator 已经结束了

并且 **gi_exc_state** 中的各个字段也重置了

    >>> r = f.send("handsome9")
    result 'handsome9'
    Traceback (most recent call last):
      File "<stdin>", line 30, in fib
    StopIteration

    The above exception was the direct cause of the following exception:

    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    RuntimeError: generator raised StopIteration
    >>> f.gi_frame.f_lasti
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    AttributeError: 'NoneType' object has no attribute 'f_lasti'

![example_gen_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_5.png)

### coroutine

#### 内存构造 coroutine

**coroutine** 类型和 **generator** 类型的大部分定义是相同的

**coroutine** 独有的一个字段叫做 **cr_origin**, 用来追踪当前的调用栈用的(这里面的数据都是从 **cr_frame** 对象中查找获得)

默认情况下 **cr_origin** 是不启动的, 需要通过 **sys.set_coroutine_origin_tracking_depth** 去启动这个功能, 可以查看文档获得更多细节 [docs.python.org(set_coroutine_origin_tracking_depth)](https://docs.python.org/3/library/sys.html#sys.set_coroutine_origin_tracking_depth)

![layout_coro](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_coro.png)

#### 示例 coroutine

我们可以来跑一个 **coroutine** 类型的示例, 理解一下各个字段的意义

我向往常一样更改了部分源代码, 所以我的 **repr** 会打印出更多的信息

    import sys
    import time
    import asyncio

    sys.set_coroutine_origin_tracking_depth(100)
    cor_list = list()


    async def cor(recursive_depth=1):
        t1 = time.time()
        try:
            await asyncio.sleep(3)
            1 / 0
        except ZeroDivisionError:
            if recursive_depth > 0:
                r = cor(recursive_depth-1)
                cor_list.append(r)
                await r
        t2 = time.time()
        print("recursive_depth: %d, cost %.2f seconds" % (recursive_depth, t2 - t1))


    def pr_cor_list():
        for index, each in enumerate(cor_list):
            print("index: %d, id: %d, each.cr_frame.f_lasti: %s" % (index, id(each), "None object" if each.cr_frame is None else str(each.cr_frame.f_lasti)))
            print(repr(each))


    async def test():
        c = cor()
        cor_list.append(c)
        ts = time.time()
        pending = [c]
        pr_cor_list()
        while pending:
            done, pending = await asyncio.wait(pending, timeout=2)
            ts_now = time.time()
            print("%.2f seconds elapse" % (ts_now - ts, ))
            pr_cor_list()

    if __name__ == "__main__":
        asyncio.run(test())

你调用一个通过 **async** 定义的函数的时候, 产生的是一个类型为 **coroutine** 的对象

    >>> c = cor()
    >>> type(c)
    <class 'coroutine'>

在 **test** 函数中, 第一个 **await** 声明之前的瞬间

这是我电脑中字段 **cr_origin** 中的内容, 是自下而上的调用栈信息

    >>> cor_list[0].cr_origin
    (('<stdin>', 2, 'test'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/events.py', 81, '_run'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 1765, '_run_once'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 544, 'run_forever'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 576, 'run_until_complete'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/runners.py', 43, 'run'), ('<stdin>', 2, '<module>'))


![example_coro_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_0.png)

在第 2.01 秒时, 各个字段中的内容并未发生改变, 此时 **coroutine.cr_frame** 中的 **f_lasti** 指向了 **cor** 函数中第一个 **await** 的位置

![example_coro_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_1.png)

在第 4.01 秒时, cor_list[0] 中的 **f_lasti** 指向了 **await r** 这个位置, 值为 86

**exc_type**, **exc_value** and **exc_traceback** 保存了 **ZeroDivisionError** 的信息, 和 **generator** 对象的处理方式相同

cor_list[1] 现在停在了 **await asyncio.sleep(3)** 这个位置上, **f_lasti** 中的值为 20

cor_list[1] 的 **cr_code** 和 cor_list[0] 的 **cr_code** 相同, 但是 **cr_frame** 却不同

每一个函数调用都会产生一个新的 frame 对象与之关联, python 虚拟机中的调用栈机制和 [Stack frame](http://en.citizendium.org/wiki/Stack_frame) 中的类似

![example_coro_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_2.png)

    >>> cor_list[1].cr_origin
    (('<stdin>', 8, 'cor'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/events.py', 81, '_run'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 1765, '_run_once'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 544, 'run_forever'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 576, 'run_until_complete'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/runners.py', 43, 'run'), ('<stdin>', 2, '<module>'))

在 6.01 秒时, **cor_list[0]** 和 **cor_list[1]** 都结束并返回了, 他们的 **cr_frame** 都为空指针, 处理方式和 **generator** 类型类似

![example_coro_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_3.png)

### async generator

#### 内存构造 async generator

除了 **ag_finalizer**, **ag_hooks_inited** and **ag_closed** 这三个额外添加的字段, **async generator** 的构造和 **generator** 是相同的

![layout_async_gen](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_async_gen.png)

#### 示例 async generator

**set_asyncgen_hooks** 函数可以设置一个 **firstiter** 和一个 **finalizer**, **firstiter** 会在**async generator** 第一次迭代之前调用, **finalizer** 会在垃圾回收之前进行调用

**asyncio base event loop** 中的 **run_forever** 函数做了如下定义

    def run_forever(self):
        ...
        old_agen_hooks = sys.get_asyncgen_hooks()
        sys.set_asyncgen_hooks(firstiter=self._asyncgen_firstiter_hook,
                               finalizer=self._asyncgen_finalizer_hook)
        try:
            ...
        finally:
            sys.set_asyncgen_hooks(*old_agen_hooks)

你也可以定义你自己的 **firstiter** 和 **finalizer**, 更多详细信息参考 [ython3-doc set_asyncgen_hooks](https://docs.python.org/3/library/sys.html#sys.set_asyncgen_hooks)

	# example of set_asyncgen_hooks
    import sys

    async def async_fib(n):
    	yield 1

    def firstiter(async_gen):
        print("in firstiter: ", async_gen)

    def finalizer(async_gen):
        print("in finalizer: ", async_gen)

    sys.set_asyncgen_hooks(firstiter, finalizer)
    >>> f = async_fib(3)
	>>> f.__anext__()
	in firstiter:  <async_generator object async_fib at 0x10a98f598>
    <async_generator_asend at 0x10a7487c8>

我们来定义一个 async iterator 并尝试一步步迭代

    import asyncio

    async def async_fib(n):
        t = 0
        i = 1
        j = 1
        r = 0
        result = None
        while t <= n:
            print("result", repr(result))
            await asyncio.sleep(3)
            if t < 2:
                result = yield i
            else:
                r = i + j
                result = yield r
                i = j
                j = r
            t += 1

    class AsendTest(object):
        def __init__(self, n):
            self.f = async_fib(n)
            self.loop = asyncio.get_event_loop()

        async def make_the_call(self, val):
            r = await self.f.asend(val)
            print("repr asend", repr(r))

        def __call__(self, *args, **kwargs):
            self.loop.run_until_complete(self.make_the_call(args[0]))

    a = AsendTest(3)
	>>> type(a.f)
	<class 'async_generator'>

![example_async_gen0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_0.png)

开始迭代

如果你需要 _\_aiter_\_, _\_anext_\_ 等函数的相关信息, 可以参考 [pep-0525](https://www.python.org/dev/peps/pep-0525/)

    >>> a(None)
    result None
    repr asend 1
	>>> a.f.ag_frame.f_lasti
	68

**ag_weakreflist** 指向了一个 **BaseEventLoop(asyncio->base_events.py)** 创建的弱引用, loop 需要保留与之相关的所有 **async generator** 的信息, 这样在出现异常/退出的时候可以把这些活动中的 **async generator** 通通关掉, 可以读这部分代码看看 [source code](https://github.com/python/cpython/blob/3.7/Lib/asyncio/base_events.py)

**ag_finalizer** n现在指向了一个 **finalizer**, 设个 **finalizer** 是被 BaseEventLoop 通过 **sys.set_asyncgen_hooks** 方法配置的

**ag_hooks_inited** 为 1, 标明当前的 hooks是已配置的状态

![example_async_gen1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_1.png)

第二次 while 循环中, 各个字段中的值未发生改变

    >>> a("handsome")
    result 'handsome'
    repr asend 1
    >>> a.f.ag_frame.f_lasti
    68

现在 **f_lasti** 指向了函数 **async_fib** 的第二个 **yield** 的位置

    >>> a("handsome2")
    result 'handsome2'
    repr asend 2
    >>> a.f.ag_frame.f_lasti
    84

![example_async_gen2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_2.png)

    >>> a("handsome3")
    result 'handsome3'
    repr asend 3
    >>> a.f.ag_frame.f_lasti
    84
    >>> a("handsome4")
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
      File "<stdin>", line 9, in __call__
      File "/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py", line 589, in run_until_complete
        return future.result()
      File "<stdin>", line 6, in make_the_call
    StopAsyncIteration

现在 **ag_closed** 被设置为 1(只有在 async generator 抛出了 **StopAsyncIteration** 异常, 或者 关联的 aclose() 方法被调用的情况下会被设置为 1)

并且 **ag_frame** 被释放了

![example_async_gen3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_3.png)

#### free list

在类型 **async_generator_asend** 和 **async_generator_wrapped_value** 上面使用了free list(缓冲池)机制

    #ifndef _PyAsyncGen_MAXFREELIST
    #define _PyAsyncGen_MAXFREELIST 80
    #endif
    static _PyAsyncGenWrappedValue *ag_value_freelist[_PyAsyncGen_MAXFREELIST];
    static int ag_value_freelist_free = 0;

    static PyAsyncGenASend *ag_asend_freelist[_PyAsyncGen_MAXFREELIST];
    static int ag_asend_freelist_free = 0;

因为这两个类型存货的时间一般都很短, 并且在每一个 **_\_anext_\_** 调用的时候都会实例化他们, 缓冲池机制可以
* 提高 6-10% 的性能
* 减小内存碎片

两个 r 的 id 是相同的, 同个**async_generator_asend**  对象被重复的进行了使用

    >>> f = async_fib(3)
    >>> r = f.asend(None)
    >>> type(r)
    <class 'async_generator_asend'>
    >>> id(r)
    4376804088
    >>> del r
	>>> r = f.asend(None)
	>>> id(r)
	4376804088

![free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/free_list.png)