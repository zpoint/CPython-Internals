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

