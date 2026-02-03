# code

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [fields](#fields)
    * [example](#example)
    * [co_code](#co_code)
    * [co_lnotab and co_firstlineno](#co_lnotab-and-co_firstlineno)
    * [co_zombieframe](#co_zombieframe)
    * [co_extra](#co_extra)

# related file
* cpython/Objects/codeobject.c
* cpython/Include/codeobject.h

```python3


```

# memory layout

> a code object is CPython's internal representation of a piece of runnable Python code, such as a function, a module, a class body, or a generator expression. When you run a piece of code, it is parsed and compiled into a code object, which is then run by the CPython virtual machine (VM)

For more detail, please refer to [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python)

![layout](https://img-blog.csdnimg.cn/20190208112516130.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# fields

## example

Let's run an example written in Python

```python3
def a(x, y, *args,  z=3, **kwargs):
    def b():
        bbb = 4
        print(x, k, bbb)
    k = 4
    print(x, y, args, kwargs)

print("co_argcount", a.__code__.co_argcount)
print("co_kwonlyargcount", a.__code__.co_kwonlyargcount)
print("co_nlocals", a.__code__.co_nlocals)
print("co_stacksize", a.__code__.co_stacksize)
print("co_flags", a.__code__.co_flags)
print("co_firstlineno", a.__code__.co_firstlineno)
print("co_code", a.__code__.co_code)
print("co_consts", a.__code__.co_consts)
print("co_names", a.__code__.co_names)
print("co_varnames", a.__code__.co_varnames)
print("co_freevars", a.__code__.co_freevars)
print("co_cellvars", a.__code__.co_cellvars)

print("co_cell2arg", a.__code__.co_filename)
print("co_name", a.__code__.co_name)
print("co_lnotab", a.__code__.co_lnotab)

print("\n\n", a.__code__.co_consts[1])
print("co_freevars", a.__code__.co_consts[1].co_freevars)
print("co_cellvars", a.__code__.co_consts[1].co_cellvars)



```

Output

```python3
co_argcount 2
co_kwonlyargcount 1
co_nlocals 6
co_stacksize 5
co_flags 15
co_firstlineno 1
co_code b'\x87\x00\x87\x01f\x02d\x01d\x02\x84\x08}\x05d\x03\x89\x00t\x00\x88\x01|\x01|\x03|\x04\x83\x04\x01\x00d\x00S\x00'
co_consts (None, <code object b at 0x10228e810, file "/Users/zpoint/Desktop/cpython/bad.py", line 2>, 'a.<locals>.b', 4)
co_names ('print',)
co_varnames ('x', 'y', 'z', 'args', 'kwargs', 'b')
co_freevars ()
co_cellvars ('k', 'x')
co_cell2arg /Users/zpoint/Desktop/cpython/bad.py
co_name a
co_lnotab b'\x00\x01\x0e\x03\x04\x01'

<code object b at 0x10228e810, file "/Users/zpoint/Desktop/cpython/bad.py", line 2>
co_freevars ('k', 'x')
co_cellvars ()


```

from the output and the answers in [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) we can know the meaning of some fields

**co_argcount**

> The number of arguments that the function takes, excluding any `*args` and `**kwargs`. Function calls in bytecode work by pushing all of the arguments onto the stack and then invoking CALL_FUNCTION; the co_argcount can then be used to determine whether the function was passed the right number of variables.

**co_kwonlyargcount**

> The number of [keyword-only arguments](https://www.python.org/dev/peps/pep-3102/)

**co_nlocals**

it's related to the **f_localsplus** in [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)

> The number of local variables in the function. As far as I can tell this is just the length of co_varnames. This is presumably used in order to decide how much space to allocate for local variables when the function is called.

**co_stacksize**

it's related to the **f_valuestack** in [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)

> An integer representing the maximum amount of stack space that the function will use. This is necessary because the VM stack associated with the code object is pre-allocated when the code is called. Thus, if co_stacksize is too low, the function may overrun its allocated stack and terrible things happen.

**co_flags**

> an integer that combines a number of boolean flags about the function

**co_consts**

> This is a tuple of all constants used in the function, like integers, strings, and booleans. It is used by the LOAD_CONST opcode, which takes an argument indicating the index in the co_consts tuple to load from.

**co_names**

> A tuple of strings used in the code object as attributes, global variable names, and imported names. Opcodes that use one of these names (e.g., LOAD_ATTR) take as an argument an integer index into this tuple. These are in order of first use.

**co_varnames**

> A tuple with the names of all of the function's local variables, including arguments. It contains first normal arguments, then the names of the *args and *\**kwargs arguments if applicable, then other local variables in order of first use.

**co_cellvars** and **co_freevars**

> These two are used for implementing nested function scopes. co_cellvars is a tuple containing the names of all variables in the function that are also used in a nested function, and co_freevars has the names of all variables used in the function that are defined in an enclosing function scope.

## co_code

when you enter the command  `./python.exe -m dis code.py`

the **_unpack_opargs** in `Lib/dis.py` will do the translation

if you check the file in `Include/opcode.h`, you will find `#define HAVE_ARGUMENT            90` and `#define HAS_ARG(op) ((op) >= HAVE_ARGUMENT)`, which means opcode with a value greater than **90** has arguments, while opcode with a value less than **90** doesn't

```python3
def _unpack_opargs(code):
    # code example: b'd\x01}\x00t\x00\x88\x01\x88\x00|\x00\x83\x03\x01\x00d\x00S\x00'
    extended_arg = 0
    for i in range(0, len(code), 2):
        op = code[i]
        if op >= HAVE_ARGUMENT:
            arg = code[i+1] | extended_arg
            extended_arg = (arg << 8) if op == EXTENDED_ARG else 0
        else:
            arg = None
        # yield example: 0 100 1
        yield (i, op, arg)

```

So, **co_code** is the opcode and argument stored in binary format

```python3
>>> c = b'd\x01}\x00t\x00\x88\x01\x88\x00|\x00\x83\x03\x01\x00d\x00S\x00'
>>> c = list(bytearray(c))
>>> c
[100, 1, 125, 0, 116, 0, 136, 1, 136, 0, 124, 0, 131, 3, 1, 0, 100, 0, 83, 0]

```

The binary format can be translated to

```python3
0 100 1  (LOAD_CONST)
2 125 0  (STORE_FAST)
4 116 0  (LOAD_GLOBAL)
6 136 1  (LOAD_DEREF)
8 136 0  (LOAD_DEREF)
10 124 0 (LOAD_FAST)
12 131 3 (CALL_FUNCTION)
14 1 None(POP_TOP)
16 100 0 (LOAD_CONST)
18 83 None(RETURN_VALUE)

```

## co_lnotab and co_firstlineno

**co_firstlineno**

> The 1-indexed line number of the beginning of the Python code from which the code object was generated. In combination with co_lnotab, this is used to compute line information in places like exception tracebacks

**co_lnotab**

> This means line number table, and stores a compressed mapping of bytecode instructions to line numbers.

Let's see an example.

The first pair (0, 1) in **co_lnotab** means byte offset 0, line offset: 1 + co_firstlineno(7) == 8

The second pair (4, 1) in **co_lnotab** means byte offset 4, line offset 1 + 8 (previous offset) == 9

```python3
import dis

def f1(x):
    x = 3
    y = 4

def f2(x):
    x = 3
    y = 4

print(f2.__code__.co_firstlineno) # 7
print(repr(list(bytearray(f2.__code__.co_lnotab)))) # [0, 1, 4, 1]
print(dis.dis(f2))
"""
0 LOAD_CONST               1 (3)
2 STORE_FAST               0 (x)
4 LOAD_CONST               2 (4)
6 STORE_FAST               1 (y)
8 LOAD_CONST               0 (None)
"""

```

## co_zombieframe

You can read the following comment from `Objects/frameobject.c`

For more detail, please refer to [frame object(zombie frame)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md#zombie-frame)

> each code object will hold a single "zombie" frame. This retains the allocated and initialized frame object from an invocation of the code object. The zombie is reanimated the next time we need a frame object for that code object. Doing this saves the malloc/ realloc required when using a free_list frame that isn't the correct size. It also saves some field initialization.

## co_extra

This field stores a pointer to a **_PyCodeObjectExtra** object

```c
typedef struct {
    Py_ssize_t ce_size;
    void *ce_extras[1];
} _PyCodeObjectExtra;

```

Since it has a size field and an array of (void *) pointers, it can store almost everything.

Usually, it's a function pointer related to the interpreter for debug or JIT usage.

For more detail please refer to [PEP 523 -- Adding a frame evaluation API to CPython](https://www.python.org/dev/peps/pep-0523/)