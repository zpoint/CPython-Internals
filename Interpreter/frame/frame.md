# frame

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
    * [f_valuestack/f_stacktop/f_localsplus](#f_valuestack/f_stacktop/f_localsplus)
    * [f_blockstack](#f_blockstack)
    * [f_back](#f_back)
* [free_list mechanism](#free_list-mechanism)
    * [zombie frame](#zombie-frame)
    * [free_list](#free_list-sub)

# related file
* cpython/Objects/frameobject.c
* cpython/Include/frameobject.h

# memory layout

the **PyFrameObject** is the stack frame in python virtual machine, it contains space for the currently executing code object, parameters, variables in different scope, try block info and etc

for more information please refer to [stack frame strategy](http://en.citizendium.org/wiki/Memory_management)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/layout.png)

# example

every time you make a function call, a new **PyFrameObject** will be created and attached to the current function call

it's not intuitive to trace a frame object in the middle of a function, I will use a [generator object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen.md) to do the explanation

you can always get the frame of the current environment by executing `sys._current_frames()`

if you need the meaning of each field, please refer to [Junnplus' blog](https://github.com/Junnplus/blog/issues/22) or read source code directly

## f_valuestack/f_stacktop/f_localsplus

**PyFrameObject** object is variable-sized object, it can be cast to type **PyVarObject**, the real **ob_size** is decided by the **code** object

```c
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

```

the **ob_size** is the sum of code->co_stacksize, code->co_nlocals, code->co_cellvars and code->co_freevars

**code->co_stacksize**: an integer that represents the maximum amount stack space that the function will use. It's computed when the code object generated

**code->co_nlocals**: number of local variables

**code->co_cellvars**: a tuple containing the names of all variables in the function that are also used in a nested function

**code->nfrees**: the names of all variables used in the function that is defined in an enclosing function scope

for more information about **PyCodeObject** please refer to [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) and [code object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code.md)

let's see an example

```python3
def g2(a, b=1, c=2):
    yield a
    c = str(b + c)
    yield c
    new_g = range(3)
    yield from new_g

```

the **dis** result

```python3
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

```

let's iter through the generator

```python3
>>> gg = g2("param a")

```

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example0.png)

after the first **next** returns, the first **opcode** `0 LOAD_FAST                0 (a)` will be executed and the current execution flow is in the middle of the second **opcode** `2 YIELD_VALUE`

the field **f_lasti** is 2, indicate that the virtual program counter is in `2 YIELD_VALUE`

the **opcode** `LOAD_FAST` will push the paramter to the **f_valuestack**, and **opcode** `YIELD_VALUE` will **pop** the top element in the **f_valuestack**, the defination of **pop** is `#define BASIC_POP()       (*--stack_pointer)`

the value(address 0x100a5b538) in **f_valuestack** is the same as the previous step(previous picture), but the first element the address(0x100a5b538) pointed to is different, currently it's a pointer to a PyUnicodeObject('param a') or an invalid address(if the PyUnicodeObject is deallocated))

```python3
>>> next(gg)
'param a'

```

![example1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1.png)

```python3
>>> next(gg)
'3'


```

the opcode `6 LOAD_GLOBAL              0 (str)` `8 LOAD_FAST                1 (b)` and `10 LOAD_FAST                2 (c)` in line 3 pushes **str**(parameter str is stored in the frame-f_code->co_names field), **b**(int 1) and **c**(int 2) to **f_valuestack**, opcode `12 BINARY_ADD` pops off the top 2 elements in **f_valuestack**(**b** and **c**), sum these two values, store to the top of the **f_valuestack**, this is what the **f_valuestack** looks like after `12 BINARY_ADD`

![example1_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2.png)

the opcode `14 CALL_FUNCTION            1` will pop the function and argument off the stack and delegate the actual function call

after the function call, result `'3'` is pushed onto the stack

![example1_2_1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_1.png)

opcode `16 STORE_FAST               2 (c)` pops off the top element in the **f_valuestack** and stores it into the 2th position of the **f_localsplus**

![example1_2_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_2.png)

opcode `18 LOAD_FAST                2 (c)` push the 2th element in the **f_localsplus** onto the **f_valuestack**, and  `20 YIELD_VALUE ` pops it and send it to the caller

field **f_lasti** is 20, indicate that it's currently executing the opcode `20 YIELD_VALUE`

![example2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example2.png)

after `24 LOAD_GLOBAL              1 (range)` and `26 LOAD_CONST               1 (3)`

![example1_3_1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_1.png)

after `28 CALL_FUNCTION            1`

![example1_3_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_2.png)

after `30 STORE_FAST               3 (new_g)`

![example1_3_3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_3.png)

after `32 LOAD_FAST                3 (new_g)`

![example1_3_4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_4.png)

the opcode `34 GET_YIELD_FROM_ITER` makes sure the stack's top is an iterable object

`36 LOAD_CONST               0 (None)` pushes `None` onto the stack

```python3
>>> next(gg)
0

```

field **f_lasti** is 36, indicate that it's before the `38 YIELD_FROM`

![example3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example3.png)

the frame object deallocated after the **StopIteration** raised (the opcode `44 RETURN_VALUE` also executed)

```python3
>>> next(gg)
1
>>> next(gg)
2
>>> next(gg)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration
>>> repr(gg.gi_frame)
'None'

```

## f_blockstack

f_blockstack is an array, element type is **PyTryBlock**, size is **CO_MAXBLOCKS**(20)

the definition of **PyTryBlock**

```c
typedef struct {
    int b_type;                 /* what kind of block this is */
    int b_handler;              /* where to jump to find handler */
    int b_level;                /* value stack level to pop to */
} PyTryBlock;

```

let's define a generator with some blocks

```python3
def g3():
    try:
        yield 1
        1 / 0
    except ZeroDivisionError:
        yield 2
        try:
            yield 3
            import no
        except ModuleNotFoundError:
            for i in range(3):
                yield i + 4
            yield 4
        finally:
            yield 100


>>> gg = g3()

```

![blockstack0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack0.png)

in the first **yield** statement, the first **try block** is set up

**f_iblock** is 1, indicate that there's currently one block

**b_type** 122 is the opcode `SETUP_FINALLY`, **b_handler** 20 is the opcode location of the `except ZeroDivisionError`, **b_level** 0 is the stack pointer's position to use

```python3
>>> next(gg)
1

```

![blockstack1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack1.png)

**b_type** 257 is the opcode `EXCEPT_HANDLER`, `EXCEPT_HANDLER` has a special meaning

```c
/* EXCEPT_HANDLER is a special, implicit block type which is created when
   entering an except handler. It is not an opcode but we define it here
   as we want it to be available to both frameobject.c and ceval.c, while
   remaining private.*/
#define EXCEPT_HANDLER 257

```

**b_handler** set to -1, since already in the processing of the try block

**b_level** doesn't change

```python3
>>> next(gg)
2

```

![blockstack2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack2.png)

**f_iblock** is 3, the second try block comes from `finally:`(opcode position 116), and the third try block comes from `except ModuleNotFoundError:`(opcode position 62)

```python3
>>> next(gg)
3

```

![blockstack3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack3.png)

```python3
>>> next(gg)
4

```

**b_type** of the third try block becomes 257 and **b_handler** becomes -1, means this block is currently being handling

![blockstack4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack4.png)

the other two try block is handled properly

```python3
>>> next(gg)
5
>>> next(gg)
6
>>> next(gg)
4
>>> next(gg)
100

```

![blockstack5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack5.png)

frame object deallocated

```python3
>>> next(gg)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

## f_back

**f_back** is a pointer which points to the previous frame, it makes the related frames a single linked list

```python3
import inspect

def g4(depth):
    print("depth", depth)
    print(repr(inspect.currentframe()), inspect.currentframe().f_back)
    if depth > 0:
        g4(depth-1)


g4(3)

```

output

```python3
depth 3
<frame at 0x7fedc2f2e9a8, file '<input>', line 3, code g4> <frame at 0x7fedc2cab468, file '<input>', line 1, code <module>>
depth 2
<frame at 0x7fedc2de54a8, file '<input>', line 3, code g4> <frame at 0x7fedc2f2e9a8, file '<input>', line 5, code g4>
depth 1
<frame at 0x7fedc2ca6348, file '<input>', line 3, code g4> <frame at 0x7fedc2de54a8, file '<input>', line 5, code g4>
depth 0
<frame at 0x10c2c9930, file '<input>', line 3, code g4> <frame at 0x7fedc2ca6348, file '<input>', line 5, code g4>

```

![f_back](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/f_back.png)

# free_list mechanism

## zombie frame

the first time a code object attached to a frame object, after the execution of the code block, the frame object will not be freed, it becomes a "zombie" frame, next time the code block executes again, it will reuse the same frame object

the strategy saves malloc/realloc overhead and some field initialization

```python3
def g5():
    yield 1

>>> gg = g5()
>>> gg.gi_frame
<frame at 0x10224c970, file '<stdin>', line 1, code g5>
>>> next(gg)
1
>>> next(gg)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

>>> gg3 = g5()
>>> gg3.gi_frame # id same as previous one, the same frame object in the same code block is reused
<frame at 0x10224c970, file '<stdin>', line 1, code g5>

```

## free_list sub

there's a single linked list store the deallocated frame object, it saves malloc/free overhead

```c
static PyFrameObject *free_list = NULL;
static int numfree = 0;         /* number of frames currently in free_list */
/* max value for numfree */
#define PyFrame_MAXFREELIST 200

```

When a **PyFrameObject** is on the free list, only the following members have a meaning

```python3
ob_type             == &Frametype
f_back              next item on free list, or NULL
f_stacksize         size of value stack
ob_size             size of localsplus

```

the creating process will check if the stack size is enough

```c
if (Py_SIZE(f) < extras) {
    PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);

```

let's see an example

```python3
import inspect

def g6():
    yield repr(inspect.currentframe()), inspect.currentframe().f_back

>>> gg = g6()
>>> gg1 = g6()
>>> gg2 = g6()

```

![free_list0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list0.png)

the frame attached to variable **gg** is deallocated, because it's the first frame execute the code block, it becomes the "zombie" frame of the **code** object

because the **code** object still contains reference count to the frame object("zombie" frame), the frame object won't go to the free_list or trigger gc

```python3
>>> next(gg)
("<frame at 0x1052d83a0, file '<stdin>', line 2, code g6>", <frame at 0x105225e50, file '<stdin>', line 1, code <module>>)
>>> next(gg)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![free_list1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list1.png)

```python3
>>> next(gg1)
("<frame at 0x105620040, file '<stdin>', line 2, code g6>", <frame at 0x105474cc0, file '<stdin>', line 1, code <module>>)
>>> next(gg1)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![free_list2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list2.png)

```python3
>>> next(gg2)
("<frame at 0x105482d00, file '<stdin>', line 2, code g6>", <frame at 0x105225e50, file '<stdin>', line 1, code <module>>)
>>> next(gg2)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![free_list3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list3.png)
