# code

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [fields](#fields)
	* [example](#example)

#### related file
* cpython/Objects/codeobject.c
* cpython/Include/codeobject.h

#### memory layout

> a code object is CPython's internal representation of a piece of runnable Python code, such as a function, a module, a class body, or a generator expression. When you run a piece of code, it is parsed and compiled into a code object, which is then run by the CPython virtual machine (VM)

for more detail, please refer to [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python)

![layout](https://img-blog.csdnimg.cn/20190208112516130.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### fields

##### example

let's run an example written in python

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

output

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


from the output and the answers in [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) we can know the meaning of some fields

**co_argcount**

> The number of arguments that the function takes, excluding any *args and **kwargs. Function calls in bytecode work by pushing all of the arguments onto the stack and then invoking CALL_FUNCTION; the co_argcount can then be used to determine whether the function was passed the right number of variables.

**co_kwonlyargcount**

> The number of keyword-only arguments

**co_nlocals**

> The number of local variables in the function. As far as I can tell this is just the length of co_varnames. This is presumably used in order to decide how much space to allocate for local variables when the function is called.

**co_stacksize**

it's related to the **f_valuestack** in [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)

> An integer representing the maximum amount of stack space that the function will use. This is necessary because the VM stack associated with the code object is pre-allocated when the code is called. Thus, if co_stacksize is too low, the function may overrun its allocated stack and terrible things happen.

**co_flags**

> an integer that combines a number of boolean flags about the function

**co_names**

> A tuple of strings used in the code object as attributes, global variable names, and imported names. Opcodes that use one of these names (e.g., LOAD_ATTR) take as an argument an integer index into this tuple. These are in order of first use.

**co_varnames**

> A tuple with the names of all of the function's local variables, including arguments. It contains first normal arguments, then the names of the *args and *\**kwargs arguments if applicable, then other local variables in order of first use.

**co_cellvars** and **co_freevars**

> These two are used for implementing nested function scopes. co_cellvars is a tuple containing the names of all variables in the function that are also used in a nested function, and co_freevars has the names of all variables used in the function that are defined in an enclosing function scope.

