# gen

### category

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

### related file
* cpython/Objects/genobject.c
* cpython/Include/genobject.h

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

the defination of **generator** object is less than 4 lines

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

we initialize a new generator, the **f_lasti** in **gi_frame** act as the program counter in the python virtual machine, it indicate the next instruction offset from the code block inside the **gi_code**

	>>> fib.__code__
	<code object fib at 0x1041069c0, file "<stdin>", line 1>
    >>> f.gi_code
    <code object fib at 0x1041069c0, file "<stdin>", line 1>

the **gi_code** inside the f object is the **code** object that represent the function fib

the **gi_running** is 0, indicating the generator is not executing right now

**gi_name** and **gi_qualname** all points to same **unicode** object, all fields in **gi_exc_state** have value 0x00

![example_gen_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_0.png)

    >>> r = f.send(None)
    result None
    >>> f.gi_frame.f_lasti
    52
	>>> repr(r)
	'1'

looing into the object f, nothing changed

but the **f_lasti** in **gi_frame** now in the position 52(the first place keyword **yield** appears)

![example_gen_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_1.png)

step one more time, due to the while loop, the **f_lasti** still points to same position

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

there will be another article talking about the [exception handling](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/exception/exception_cn.md) later, the refer link is reserved

    >>> r = f.send("handsome8")
    result 'handsome8'
    >>> f.gi_frame.f_lasti
    198
    >>> repr(r)
    "'ModuleNotFoundError finally'"

![example_gen_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_gen_4.png)

now, the **StopIteration** is raised

the frameObject in **gi_frame** field is freed

field **gi_frame** points to null pointer, indicating that the generator is terminated

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

#### memory layout coroutine

most parts of the defination of the **coroutine** type and **generator** are the same

the coroutine-only field named **cr_origin**, tracking the trackback of the **coroutine** object, is disabled by default, can be enabled by **sys.set_coroutine_origin_tracking_depth**, for more detail please refer to [docs.python.org(set_coroutine_origin_tracking_depth)](https://docs.python.org/3/library/sys.html#sys.set_coroutine_origin_tracking_depth)

![layout_coro](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/layout_coro.png)

#### example coroutine

let's try to run an example with **coroutine** type defined to understand each field's meaning

as usual, I've altered the source code so that my **repr** function is able to print all the low level detail of the object

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

if you call a function defined with the **async** keyword, the calling result is a a object of type **coroutine**

    >>> c = cor()
    >>> type(c)
    <class 'coroutine'>

in the **test** function, before the first **await** statement at the moment

it's the call stack from the bottom to top

    >>> cor_list[0].cr_origin
    (('<stdin>', 2, 'test'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/events.py', 81, '_run'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 1765, '_run_once'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 544, 'run_forever'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/base_events.py', 576, 'run_until_complete'), ('/Users/zpoint/Desktop/cpython/Lib/asyncio/runners.py', 43, 'run'), ('<stdin>', 2, '<module>'))

the content in cr_origin field in my computer



![example_coro_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/example_coro_0.png)

