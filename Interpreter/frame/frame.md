# frame

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [f_valuestack/f_stacktop/f_localsplus](#f_valuestack/f_stacktop/f_localsplus)
* [free_list](#free_list)

#### related file
* cpython/Objects/frameobject.c
* cpython/Include/frameobject.h

#### memory layout

the **PyFrameObject** is the stack frame in python virtual machine, it contains space for the current executing code object, parameters, variables in different scope, try block info and etc

for more information please refer to [stack frame strategy](http://en.citizendium.org/wiki/Memory_management)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/layout.png)

#### example

every time you make a function call, a new **PyFrameObject** will be created and attached to the current function call

it's not intuitive to trace a frame object in the middle of a function, I will use a [generator object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen.md) to do the explanation

you can always get the frame of the current environment by executing `sys._current_frames()`


##### f_valuestack/f_stacktop/f_localsplus

**PyFrameObject** object is variable-sized object, it can be cast to type **PyVarObject**, the real **ob_size** is decided by the **code** object

    Py_ssize_t extras, ncells, nfrees;
    ncells = PyTuple_GET_SIZE(code->co_cellvars);
    nfrees = PyTuple_GET_SIZE(code->co_freevars);
    extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
    /* omit */
    if (free_list == NULL) { /* omit */
        f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, extras);
    }
    else { /* omit */
    	PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);
    }
    extras = code->co_nlocals + ncells + nfrees;
    f->f_valuestack = f->f_localsplus + extras;
    for (i=0; i<extras; i++)
        f->f_localsplus[i] = NULL;

the **ob_size** is the sum of code->co_stacksize, code->co_nlocals, code->co_cellvars and code->co_freevars

**code->co_stacksize**: a integer that represent the maximum amount stack space that the function will use. It's computed when the code object generated

**code->co_nlocals**: number of local variables

**code->co_cellvars**: a tuple containing the names of all variables in the function that are also used in a nested function

**code->nfrees**: the names of all variables used in the function that are defined in an enclosing function scope

for more information about **PyCodeObject** please refer to [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) and [code object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code.md)(reserved)

let's see an example

    def g2(a, b=1, c=2):
        yield a
        c = str(b + c)
        yield c
        new_g = range(3)
        yield from new_g

the **dis** result

	  # ./python.exe -m dis frame_dis.py
      1           0 LOAD_CONST               5 ((1, 2))
                  2 LOAD_CONST               2 (<code object g2 at 0x10c495030, file "frame_dis.py", line 1>)
                  4 LOAD_CONST               3 ('g2')
                  6 MAKE_FUNCTION            1 (defaults)
                  8 STORE_NAME               0 (g2)
                 10 LOAD_CONST               4 (None)
                 12 RETURN_VALUE

    Disassembly of <code object g2 at 0x10c495030, file "frame_dis.py", line 1>:
      2           0 LOAD_FAST                0 (a)
                  2 YIELD_VALUE
                  4 POP_TOP

      3           6 LOAD_GLOBAL              0 (str)
                  8 LOAD_FAST                1 (b)
                 10 LOAD_FAST                2 (c)
                 12 BINARY_ADD
                 14 CALL_FUNCTION            1
                 16 STORE_FAST               2 (c)

      4          18 LOAD_FAST                2 (c)
                 20 YIELD_VALUE
                 22 POP_TOP

      5          24 LOAD_GLOBAL              1 (range)
                 26 LOAD_CONST               1 (3)
                 28 CALL_FUNCTION            1
                 30 STORE_FAST               3 (new_g)

      6          32 LOAD_FAST                3 (new_g)
                 34 GET_YIELD_FROM_ITER
                 36 LOAD_CONST               0 (None)
                 38 YIELD_FROM
                 40 POP_TOP
                 42 LOAD_CONST               0 (None)
                 44 RETURN_VALUE

let's iter through the generator

	>>> gg = g2("param a")

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example0.png)

after the first **next** returns, the first **opcode** `0 LOAD_FAST                0 (a)` will be executed and the current execution flow is in the middle of the second **opcode** `2 YIELD_VALUE`

the field **f_lasti** is 2, indicate that the virtual program counter is in `2 YIELD_VALUE`

the **opcode** `LOAD_FAST` will push the paramter to the **f_valuestack**, and **opcode** `YIELD_VALUE` will **pop** the top element in the **f_valuestack**, the defination of **pop** is `#define BASIC_POP()       (*--stack_pointer)`

the value in **f_stacktop** is the same as the previous picture, but the element is the first **f_valuestack** is not null


    >>> next(gg)
    'param a'

![example1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1.png)

    >>> next(gg)
    '3'


the opcode `6 LOAD_GLOBAL              0 (str)` `8 LOAD_FAST                1 (b)` and `10 LOAD_FAST                2 (c)` in line 3 pushes **str**(parameter str is stored in the frame-f_code->>co_names field), **b**(int 1) and **c**(int 2) to **f_valuestack**, opcode `12 BINARY_ADD` pops off the top 2 elements in **f_valuestack**(**b** and **c**), sum these two values, store to the top of the **f_valuestack**, this is what the **f_valuestack** looks like after `12 BINARY_ADD`

![example1_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2.png)

the opcode `14 CALL_FUNCTION            1` will pop the function and argument off the stack and delegate the actual function call

after the function call, result `'3'` is pushed onto the stack

![example1_2_1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_1.png)

opcode `16 STORE_FAST               2 (c)` pops off the top element in the **f_valuestack** and stores into the 2th of the **f_localsplus**

![example1_2_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_2.png)

opcode `18 LOAD_FAST                2 (c)` push the 2th element in the **f_localsplus** onto the **f_valuestack**, and  `20 YIELD_VALUE ` pops it and send it to the caller

field **f_lasti** is 20, indicate that it's current executing the opcode `20 YIELD_VALUE`

![example2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example2.png)

    >>> next(gg)
    0

