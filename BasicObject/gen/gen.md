# gen

### contents

* [related file](#related-file)
* [generator](#generator)
    * [memory layout](#memory-layout-generator)
    * [example generator](#example-generator)
* [coroutine](#coroutine)
    * [memory layout](#memory-layout-coroutine)
    * [example coroutine](#example-coroutine)
* [async generator](#async-generator)
    * [memory layout](#memory-layout-async-generator)
    * [example async generator](#example-async-generator)
    * [free list](#free-list)

### related file
* cpython/Objects/genobject.c
* cpython/Include/genobject.h

### generator

#### memory layout generator

there's a common defination among **generator**, **coroutine** and **async generator**

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

the definition of **generator** object is less than 4 lines

    typedef struct {
        /* The gi_ prefix is intended to remind of generator-iterator. */
        _PyGenObject_HEAD(gi)
    } PyGenObject;

which can be expanded to

    typedef struct {
        struct _frame *gi_frame;
        char gi_running;
        PyObject *gi_code;
        PyObject *gi_weakreflist;
        PyObject *gi_name;
        PyObject *gi_qualname;
        _PyErr_StackItem gi_exc_state;
    } PyGenObject;

we can draw the layout according to the code now

![layout_gen](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_gen.png)

#### example generator

let's define and iter through a generator

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

we initialize a new generator, the **f_lasti** in **gi_frame** act as the program counter in the python virtual machine, it indicates the next instruction offset from the code block inside the **gi_code**

    >>> fib.__code__
    <code object fib at 0x1041069c0, file "<stdin>", line 1>
    >>> f.gi_code
    <code object fib at 0x1041069c0, file "<stdin>", line 1>

the **gi_code** inside the f object is the **code** object that represents the function fib

the **gi_running** is 0, indicating the generator is not executing right now

**gi_name** and **gi_qualname** all points to same **unicode** object, all fields in **gi_exc_state** have value 0x00

![example_gen_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_0.png)

    >>> r = f.send(None)
    result None
    >>> f.gi_frame.f_lasti
    52
    >>> repr(r)
    '1'

looking into the object f, nothing changed

but the **f_lasti** in **gi_frame** now in the position 52(the first place keyword **yield** appears)

![example_gen_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_1.png)

step one more time, due to the while loop, the **f_lasti** still points to the same position

    >>> r = f.send("handsome")
    result 'handsome'
    >>> f.gi_frame.f_lasti
    52
    >>> repr(r)
    '1'

send again, the **f_lasti** indicate the code offset in the position of second **yield**

    >>> r = f.send("handsome2")
    result 'handsome2'
    >>> f.gi_frame.f_lasti
    68
    >>> repr(r)
    '2'

repeat

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

now, the while loop terminated by the break statement

the **f_lasti** is in the position of the first **except** statement, the **exc_type** points to the type of the exception, **exc_value** points to the instance of the exception, and **exc_traceback** points to the traceback object

    >>> r = f.send("handsome6")
    >>> f.gi_frame.f_lasti
    120
    >>> repr(r)
    "'ZeroDivisionError'"

![example_gen_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_2.png)

the **f_lasti** is in the position of the second **except** statement, **exc_type**, **exc_value**, and **exc_traceback** now relate to ModuleNotFoundError

    >>> r = f.send("handsome7")
    'handsome7'
    >>> f.gi_frame.f_lasti
    168
    >>> repr(r)
    "'ModuleNotFoundError'"

![example_gen_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_3.png)

the **f_lasti** is in the position of the first **finally** statement, the ModuleNotFoundError is handled properly, at the top of the exception stack is the **ZeroDivisionError**

actually the information about [exception handling](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/exception.md) is stored in [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md), **gi_exec_state** is used for representing whether current generator ojbect is handling exception and the detail of the most nested exception

    >>> r = f.send("handsome8")
    result 'handsome8'
    >>> f.gi_frame.f_lasti
    198
    >>> repr(r)
    "'ModuleNotFoundError finally'"

![example_gen_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_4.png)

now, the **StopIteration** is raised

the frameObject in **gi_frame** field is freed

field **gi_frame** points to a null pointer, indicating that the generator is terminated

and states in **gi_exc_state** is restored

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

#### memory layout coroutine

most parts of the definition of the **coroutine** type and **generator** are the same

the coroutine-only field named **cr_origin**, tracking the trackback of the **coroutine** object, is disabled by default, can be enabled by **sys.set_coroutine_origin_tracking_depth**, for more detail please refer to [docs.python.org(set_coroutine_origin_tracking_depth)](https://docs.python.org/3/library/sys.html#sys.set_coroutine_origin_tracking_depth)

![layout_coro](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_coro.png)

#### example coroutine

let's try to run an example with **coroutine** type defined to understand each field's meaning

as usual, I've altered the source code so that my **repr** function is able to print all the low-level detail of the object

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

if you call a function defined with the **async** keyword, the calling result is an object of type **coroutine**

    >>> c = cor()
    >>> type(c)
    <class 'coroutine'>

in the **test** function, before the first **await** statement at the moment


    >>> cor_list[0].cr_origin
    (('<stdin>', 2, 'test'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/events.py', 81, '_run'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 1765, '_run_once'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 544, 'run_forever'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 576, 'run_until_complete'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/runners.py', 43, 'run'), ('<stdin>', 2, '<module>'))

the content in field **cr_origin** in my computer is the calling stack from bottom to top

![example_coro_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_0.png)

in the 2.01 seconds, nothing changed, except the **f_lasti** in the **coroutine.cr_frame** now points to the first **await** statement in **cor** function

![example_coro_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_1.png)

in the 4.01 seconds, **f_lasti** in cor_list[0] now points to the **await r** expression, which in position 86

the **exc_type**, **exc_value** and **exc_traceback** holds information about the **ZeroDivisionError**, same as the generator object

the coroutine in cor_list[1] now stuck in the **await asyncio.sleep(3)** expression, the value in **f_lasti** is 20

the **cr_code** is the same as cor_list[0], but the **cr_frame** is different

every function call will create a new frame, more information about [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)

![example_coro_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_2.png)

    >>> cor_list[1].cr_origin
    (('<stdin>', 8, 'cor'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/events.py', 81, '_run'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 1765, '_run_once'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 544, 'run_forever'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 576, 'run_until_complete'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/runners.py', 43, 'run'), ('<stdin>', 2, '<module>'))

in the 6.01 seconds, both **cor_list[0]** and **cor_list[1]** returned, and their **cr_frame** field becomes null pointer, the handling process is similar to the **generator** type

![example_coro_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_3.png)

### async generator

#### memory layout async generator

the layout of **async generator** is the same as **generator** type, except for the **ag_finalizer**, **ag_hooks_inited** and **ag_closed**

![layout_async_gen](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_async_gen.png)

#### example async generator

the **set_asyncgen_hooks** function is used for setting up a **firstiter** and a **finalizer**, **firstiter** will be called before when an asynchronous generator is iterated for the first time, finalizer will be called when asynchronous generator is about to be gc

the **run_forever** function in asyncio base event loop has defined

    def run_forever(self):
        ...
        old_agen_hooks = sys.get_asyncgen_hooks()
        sys.set_asyncgen_hooks(firstiter=self._asyncgen_firstiter_hook,
                               finalizer=self._asyncgen_finalizer_hook)
        try:
            ...
        finally:
            sys.set_asyncgen_hooks(*old_agen_hooks)

you can define your own event loop to override the default **firstiter** and **finalizer**, please refer to [poython3-doc set_asyncgen_hooks](https://docs.python.org/3/library/sys.html#sys.set_asyncgen_hooks) for more detail

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

let's define and iter through an async iterator

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

iterate through it

if you need more detail of `__aiter__`, `__anext__` and etc, please refer to [pep-0525](https://www.python.org/dev/peps/pep-0525/)

    >>> a(None)
    result None
    repr asend 1
    >>> a.f.ag_frame.f_lasti
    68

the **ag_weakreflist** points to a weak reference created by **BaseEventLoop(`asyncio->base_events.py`)**

it's used for shutdown all active asynchronous generators, read the [source code](https://github.com/python/cpython/blob/3.7/Lib/asyncio/base_events.py) for more detail

**ag_finalizer** now points to the **finalizer**, set up by BaseEventLoop(calling the **sys.set_asyncgen_hooks** method)

**ag_hooks_inited** is 1, indicate that hooks are set up

![example_async_gen1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_1.png)

in the second time of the while loop, nothing changed

    >>> a("handsome")
    result 'handsome'
    repr asend 1
    >>> a.f.ag_frame.f_lasti
    68

now, the **f_lasti** indicate the position of the second **yield** stateement in the function **async_fib**

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

now, the **ag_closed** is set to 1 because of the termination of the async generator(**StopAsyncIteration** raised or `aclose()` is called)

the **ag_frame** is deallocated

![example_async_gen3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_async_gen_3.png)

#### free list

the free list mechanism is used for type **async_generator_asend** and **async_generator_wrapped_value**

    #ifndef _PyAsyncGen_MAXFREELIST
    #define _PyAsyncGen_MAXFREELIST 80
    #endif
    static _PyAsyncGenWrappedValue *ag_value_freelist[_PyAsyncGen_MAXFREELIST];
    static int ag_value_freelist_free = 0;

    static PyAsyncGenASend *ag_asend_freelist[_PyAsyncGen_MAXFREELIST];
    static int ag_asend_freelist_free = 0;

because they both are short-living objects and are instantiated for every **_\_anext_\_** call, free list are able to
* boost performance 6-10%
* reduce memory fragmentation

the id is the same, the address of previous **async_generator_asend** is reused

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