# exception![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/exception)

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [exception handling](#exception-handling)

# related file

* cpython/Include/cpython/pyerrors.h
* cpython/Include/pyerrors.h
* cpython/Objects/exceptions.c
* cpython/Lib/test/exception_hierarchy.txt

# memory layout

there are various exception types defined in `Include/cpython/pyerrors.h`, the most widely used **PyBaseExceptionObject**(also the base part of any other exception type)

![base_exception](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/base_exception.png)

all other basic exception types defined in same C file are shown

![error_layout1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout1.png)

![error_layout2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout2.png)

what's following the definition of the basic exception types above are the definition of derived exception types according to exception hierarchy, because there're dozens of derived exceptions need to be defined, some **marco** are used for shortening the codes

```c
#define SimpleExtendsException(EXCBASE, EXCNAME, EXCDOC) \
static PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # EXCNAME, \
    sizeof(PyBaseExceptionObject), \
    0, (destructor)BaseException_dealloc, 0, 0, 0, 0, 0, 0, 0, \
    0, 0, 0, 0, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)BaseException_traverse, \
    (inquiry)BaseException_clear, 0, 0, 0, 0, 0, 0, 0, &_ ## EXCBASE, \
    0, 0, 0, offsetof(PyBaseExceptionObject, dict), \
    (initproc)BaseException_init, 0, BaseException_new,\
}; \
PyObject *PyExc_ ## EXCNAME = (PyObject *)&_PyExc_ ## EXCNAME

#define MiddlingExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCDOC) /* omit */

#define ComplexExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCNEW, \
                                EXCMETHODS, EXCMEMBERS, EXCGETSET, \
                                EXCSTR, EXCDOC) \ /* omit */

/*
 *    Exception extends BaseException
 */
SimpleExtendsException(PyExc_BaseException, Exception,
                       "Common base class for all non-exit exceptions.");

/*
 *    TypeError extends Exception
 */
SimpleExtendsException(PyExc_Exception, TypeError,
                       "Inappropriate argument type.");

/*
 *    StopAsyncIteration extends Exception
 */
SimpleExtendsException(PyExc_Exception, StopAsyncIteration,
                       "Signal the end from iterator.__anext__().");

```

you can find the exception hierarchy in `Lib/test/exception_hierarchy.txt`

```python3
BaseException
 +-- SystemExit
 +-- KeyboardInterrupt
 +-- GeneratorExit
 +-- Exception
      +-- StopIteration
      +-- StopAsyncIteration
      +-- ArithmeticError
      |    +-- FloatingPointError
      |    +-- OverflowError
      |    +-- ZeroDivisionError
      +-- AssertionError
      +-- AttributeError
      +-- BufferError
      +-- EOFError
      +-- ImportError
      |    +-- ModuleNotFoundError
      +-- LookupError
      |    +-- IndexError
      |    +-- KeyError
      +-- MemoryError
      +-- NameError
      |    +-- UnboundLocalError
      +-- OSError
      |    +-- BlockingIOError
      |    +-- ChildProcessError
      |    +-- ConnectionError
      |    |    +-- BrokenPipeError
      |    |    +-- ConnectionAbortedError
      |    |    +-- ConnectionRefusedError
      |    |    +-- ConnectionResetError
      |    +-- FileExistsError
      |    +-- FileNotFoundError
      |    +-- InterruptedError
      |    +-- IsADirectoryError
      |    +-- NotADirectoryError
      |    +-- PermissionError
      |    +-- ProcessLookupError
      |    +-- TimeoutError
      +-- ReferenceError
      +-- RuntimeError
      |    +-- NotImplementedError
      |    +-- RecursionError
      +-- SyntaxError
      |    +-- IndentationError
      |         +-- TabError
      +-- SystemError
      +-- TypeError
      +-- ValueError
      |    +-- UnicodeError
      |         +-- UnicodeDecodeError
      |         +-- UnicodeEncodeError
      |         +-- UnicodeTranslateError
      +-- Warning
           +-- DeprecationWarning
           +-- PendingDeprecationWarning
           +-- RuntimeWarning
           +-- SyntaxWarning
           +-- UserWarning
           +-- FutureWarning
           +-- ImportWarning
           +-- UnicodeWarning
           +-- BytesWarning
           +-- ResourceWarning

```

# exception handling

let's define an example

```python3
import sys

def t():  # position 3
    f = sys._current_frames()
    print("position 3\n\n", repr(f))
    try:  # position 4
        print("position 4\n\n", repr(f))
        1 / 0
    except Exception as e1:  # position 5
        print("position 5\n\n", repr(f))
        try:  # position 6
            print("position 6\n\n", repr(f))
            import no
        except Exception as e2:  # position 7
            print("position 7\n\n", repr(f))
            pass
        finally:  # position 8
            print("position 8\n\n", repr(f))
            pass
    finally:  # position 9
        print("position 9\n\n", repr(f))
        raise ValueError
    print("t1 done")


def t2():  # position 1
    f = sys._current_frames()
    print("position 1\n\n", repr(f))
    try:  # position 2
        print("position 2\n\n", repr(f))
        t()
    except Exception as e:  # position 10
        print("position 10\n\n", repr(f))
        print("t2", e)


```

and try to compile it

```python3
python.exe -m dis .\test.py
  1           0 LOAD_CONST               0 (0)
              2 LOAD_CONST               1 (None)
              4 IMPORT_NAME              0 (sys)
              6 STORE_NAME               0 (sys)

  4           8 LOAD_CONST               2 (<code object t at 0x10b700030, file "test.py", line 4>)
             10 LOAD_CONST               3 ('t')
             12 MAKE_FUNCTION            0
             14 STORE_NAME               1 (t)

 27          16 LOAD_CONST               4 (<code object t2 at 0x10b710b70, file "test.py", line 27>)
             18 LOAD_CONST               5 ('t2')
             20 MAKE_FUNCTION            0
             22 STORE_NAME               2 (t2)
             24 LOAD_CONST               1 (None)
             26 RETURN_VALUE

Disassembly of <code object t at 0x10b700030, file "test.py", line 4>:
  5           0 LOAD_GLOBAL              0 (sys)
              2 LOAD_METHOD              1 (_current_frames)
              4 CALL_METHOD              0
              6 STORE_FAST               0 (f)

  6           8 LOAD_GLOBAL              2 (print)
             10 LOAD_CONST               1 ('position 3\n\n')
             12 LOAD_GLOBAL              3 (repr)
             14 LOAD_FAST                0 (f)
             16 CALL_FUNCTION            1
             18 CALL_FUNCTION            2
             20 POP_TOP

  7          22 SETUP_FINALLY          178 (to 202)
             24 SETUP_FINALLY           26 (to 52)

  8          26 LOAD_GLOBAL              2 (print)
             28 LOAD_CONST               2 ('position 4\n\n')
             30 LOAD_GLOBAL              3 (repr)
             32 LOAD_FAST                0 (f)
             34 CALL_FUNCTION            1
             36 CALL_FUNCTION            2
             38 POP_TOP

  9          40 LOAD_CONST               3 (1)
             42 LOAD_CONST               4 (0)
             44 BINARY_TRUE_DIVIDE
             46 POP_TOP
             48 POP_BLOCK
             50 JUMP_FORWARD           146 (to 198)

 10     >>   52 DUP_TOP
             54 LOAD_GLOBAL              4 (Exception)
             56 COMPARE_OP              10 (exception match)
             58 POP_JUMP_IF_FALSE      196
             60 POP_TOP
             62 STORE_FAST               1 (e1)
             64 POP_TOP
             66 SETUP_FINALLY          116 (to 184)

 11          68 LOAD_GLOBAL              2 (print)
             70 LOAD_CONST               5 ('position 5\n\n')
             72 LOAD_GLOBAL              3 (repr)
             74 LOAD_FAST                0 (f)
             76 CALL_FUNCTION            1
             78 CALL_FUNCTION            2
             80 POP_TOP

 12          82 SETUP_FINALLY           80 (to 164)
             84 SETUP_FINALLY           26 (to 112)

 13          86 LOAD_GLOBAL              2 (print)
             88 LOAD_CONST               6 ('position 6\n\n')
             90 LOAD_GLOBAL              3 (repr)
             92 LOAD_FAST                0 (f)
             94 CALL_FUNCTION            1
             96 CALL_FUNCTION            2
             98 POP_TOP

 14         100 LOAD_CONST               4 (0)
            102 LOAD_CONST               0 (None)
            104 IMPORT_NAME              5 (no)
            106 STORE_FAST               2 (no)
            108 POP_BLOCK
            110 JUMP_FORWARD            48 (to 160)

 15     >>  112 DUP_TOP
            114 LOAD_GLOBAL              4 (Exception)
            116 COMPARE_OP              10 (exception match)
            118 POP_JUMP_IF_FALSE      158
            120 POP_TOP
            122 STORE_FAST               3 (e2)
            124 POP_TOP
            126 SETUP_FINALLY           18 (to 146)

 16         128 LOAD_GLOBAL              2 (print)
            130 LOAD_CONST               7 ('position 7\n\n')
            132 LOAD_GLOBAL              3 (repr)
            134 LOAD_FAST                0 (f)
            136 CALL_FUNCTION            1
            138 CALL_FUNCTION            2
            140 POP_TOP

 17         142 POP_BLOCK
            144 BEGIN_FINALLY
        >>  146 LOAD_CONST               0 (None)
            148 STORE_FAST               3 (e2)
            150 DELETE_FAST              3 (e2)
            152 END_FINALLY
            154 POP_EXCEPT
            156 JUMP_FORWARD             2 (to 160)
        >>  158 END_FINALLY
        >>  160 POP_BLOCK
            162 BEGIN_FINALLY

 19     >>  164 LOAD_GLOBAL              2 (print)
            166 LOAD_CONST               8 ('position 8\n\n')
            168 LOAD_GLOBAL              3 (repr)
            170 LOAD_FAST                0 (f)
            172 CALL_FUNCTION            1
            174 CALL_FUNCTION            2
            176 POP_TOP

 20         178 END_FINALLY
            180 POP_BLOCK
            182 BEGIN_FINALLY
        >>  184 LOAD_CONST               0 (None)
            186 STORE_FAST               1 (e1)
            188 DELETE_FAST              1 (e1)
            190 END_FINALLY
            192 POP_EXCEPT
            194 JUMP_FORWARD             2 (to 198)
        >>  196 END_FINALLY
        >>  198 POP_BLOCK
            200 BEGIN_FINALLY

 22     >>  202 LOAD_GLOBAL              2 (print)
            204 LOAD_CONST               9 ('position 9\n\n')
            206 LOAD_GLOBAL              3 (repr)
            208 LOAD_FAST                0 (f)
            210 CALL_FUNCTION            1
            212 CALL_FUNCTION            2
            214 POP_TOP

 23         216 LOAD_GLOBAL              6 (ValueError)
            218 RAISE_VARARGS            1
            220 END_FINALLY

 24         222 LOAD_GLOBAL              2 (print)
            224 LOAD_CONST              10 ('t1 done')
            226 CALL_FUNCTION            1
            228 POP_TOP
            230 LOAD_CONST               0 (None)
            232 RETURN_VALUE

Disassembly of <code object t2 at 0x10b710b70, file "test.py", line 27>:
 28           0 LOAD_GLOBAL              0 (sys)
              2 LOAD_METHOD              1 (_current_frames)
              4 CALL_METHOD              0
              6 STORE_FAST               0 (f)

 29           8 LOAD_GLOBAL              2 (print)
             10 LOAD_CONST               1 ('position 1\n\n')
             12 LOAD_GLOBAL              3 (repr)
             14 LOAD_FAST                0 (f)
             16 CALL_FUNCTION            1
             18 CALL_FUNCTION            2
             20 POP_TOP

 30          22 SETUP_FINALLY           24 (to 48)

 31          24 LOAD_GLOBAL              2 (print)
             26 LOAD_CONST               2 ('position 2\n\n')
             28 LOAD_GLOBAL              3 (repr)
             30 LOAD_FAST                0 (f)
             32 CALL_FUNCTION            1
             34 CALL_FUNCTION            2
             36 POP_TOP

 32          38 LOAD_GLOBAL              4 (t)
             40 CALL_FUNCTION            0
             42 POP_TOP
             44 POP_BLOCK
             46 JUMP_FORWARD            58 (to 106)

 33     >>   48 DUP_TOP
             50 LOAD_GLOBAL              5 (Exception)
             52 COMPARE_OP              10 (exception match)
             54 POP_JUMP_IF_FALSE      104
             56 POP_TOP
             58 STORE_FAST               1 (e)
             60 POP_TOP
             62 SETUP_FINALLY           28 (to 92)

 34          64 LOAD_GLOBAL              2 (print)
             66 LOAD_CONST               3 ('position 10\n\n')
             68 LOAD_GLOBAL              3 (repr)
             70 LOAD_FAST                0 (f)
             72 CALL_FUNCTION            1
             74 CALL_FUNCTION            2
             76 POP_TOP

 35          78 LOAD_GLOBAL              2 (print)
             80 LOAD_CONST               4 ('t2')
             82 LOAD_FAST                1 (e)
             84 CALL_FUNCTION            2
             86 POP_TOP
             88 POP_BLOCK
             90 BEGIN_FINALLY
        >>   92 LOAD_CONST               0 (None)
             94 STORE_FAST               1 (e)
             96 DELETE_FAST              1 (e)
             98 END_FINALLY
            100 POP_EXCEPT
            102 JUMP_FORWARD             2 (to 106)
        >>  104 END_FINALLY
        >>  106 LOAD_CONST               0 (None)
            108 RETURN_VALUE


```

we can see that the opcode `0 SETUP_FINALLY          178 (to 202)` maps to the outermost `finally` statement, 0 is the byte offset of opcode, 178 is the parameter of the `SETUP_FINALLY`, the real handler offset is calculated as `INSTR_OFFSET() + oparg`(`INSTR_OFFSET` is the byte offset of the first opcode to the next opcode(here is 24), `oparg` is the parameter 178), which adds up to 202 in result

`2 SETUP_FINALLY           26 (to 52)` maps to the first `except` statement, the byte offset of opcode is 26(`LOAD_GLOBAL`) and the parameter of the `SETUP_FINALLY` is also 26, 26(parameter) + 26(opcode offset) is 52

what does `SETUP_FINALLY` do ?

```c
/* cpython/Python/ceval.c */
case TARGET(SETUP_FINALLY): {
    /* NOTE: If you add any new block-setup opcodes that
       are not try/except/finally handlers, you may need to update the PyGen_NeedsFinalizing() function.
       */

    PyFrame_BlockSetup(f, SETUP_FINALLY, INSTR_OFFSET() + oparg,
                       STACK_LEVEL());
    DISPATCH();
}


/* cpython/Objects/frameobject.c */
void PyFrame_BlockSetup(PyFrameObject *f, int type, int handler, int level)
{
    PyTryBlock *b;
    if (f->f_iblock >= CO_MAXBLOCKS)
        Py_FatalError("XXX block stack overflow");
    b = &f->f_blockstack[f->f_iblock++];
    b->b_type = type;
    b->b_level = level;
    b->b_handler = handler;
}

```

for those who need detail of frame object, please refer to [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)

the `PyFrame_BlockSetup` will be called in the following opcode `SETUP_FINALLY`, `SETUP_ASYNC_WITH`, and `SETUP_WITH`(parameter `int type` equals `SETUP_FINALLY` in these opcode)

`RAISE_VARARGS`, `END_FINALLY` and `END_ASYNC_FOR` will call `PyFrame_BlockSetup` in some cases

what will happen if we call the function `t2()` ?

this is the definition of `PyTryBlock`

![try_block](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/try_block.png)

in `position 1`

the value `CO_MAXBLOCKS` is 20, you can't set up more than 20 blocks inside a frame(`try/finally/with/async with`)

![pos1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos1.png)

in `position 2`

`STACK_LEVEL` is defined as `#define STACK_LEVEL()     ((int)(stack_pointer - f->f_valuestack))`, it's the offset of the current `stack_pointer` to the `f_valuestack` in the current frame

![pos2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos2.png)

in `position 3`

![pos3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos3.png)

in `position 4`, the frame represents code object `t` now has two `PyTryBlock`, one for the `finally` statement located at opcode offset 202, the other for the `except` statement located at opcode offset 52

![pos4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos4.png)

in `position 5`, the `b_type` field in the second `PyTryBlock` is `EXCEPT_HANDLER`(257), and `b_handler` becomes -1

the second try block's value is changed in the opcode `BINARY_TRUE_DIVIDE`

```c
case TARGET(BINARY_TRUE_DIVIDE): {
    PyObject *divisor = POP();
    PyObject *dividend = TOP();
    PyObject *quotient = PyNumber_TrueDivide(dividend, divisor);
    Py_DECREF(dividend);
    Py_DECREF(divisor);
    SET_TOP(quotient);
    if (quotient == NULL) // error happened because the divisor is 0
        goto error;
    DISPATCH();
}
...
error:
...
exception_unwind:
    /* Unwind stacks if an exception occurred */
    while (f->f_iblock > 0) {
        /* Pop the current block. */
        PyTryBlock *b = &f->f_blockstack[--f->f_iblock];

        if (b->b_type == EXCEPT_HANDLER) {
        /* don't handle it again if it's already handled */
            UNWIND_EXCEPT_HANDLER(b);
            continue;
        }
        UNWIND_BLOCK(b);
        if (b->b_type == SETUP_FINALLY) {
            /* the exception handling process */
            PyObject *exc, *val, *tb;
            int handler = b->b_handler;
            _PyErr_StackItem *exc_info = tstate->exc_info;
            /* Beware, this invalidates all b->b_* fields */
            /* mark the block so that it will not be handled again */
            /* b_type: EXCEPT_HANDLER(257), b_handler: -1, STACK_LEVEL: (int)(stack_pointer - f->f_valuestack)) */
            PyFrame_BlockSetup(f, EXCEPT_HANDLER, -1, STACK_LEVEL());
            ...
            /* set up some state */
            goto main_loop;
        }

```

bacause `66 SETUP_FINALLY          116 (to 184)` exists before the `print("position 5\n\n")` statement, the third try block will exist

the block is created by the compiler, the opcode `184 LOAD_CONST               0 (None)` is between `position 8` and `position 9`, it does nothing except for load variable `e1` and deallocates `e1`

![pos5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos5.png)

in `position 6`, the following two opcodes create two more try block

```python3
 12          82 SETUP_FINALLY           80 (to 164)
             84 SETUP_FINALLY           26 (to 112)

```

![pos6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos6.png)

in `position 7`, `104 IMPORT_NAME              5 (no)` will raise an exception, which set the last block to handling state and begin the execution in  `15     >>  112 DUP_TOP`, down to the `126 SETUP_FINALLY           18 (to 146)`, the compiler set up another block for variable `e2`

![pos7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos7.png)

in `position 8`, all blocks in inner scope are handled properly and popped off

![pos8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos8.png)

in `position 9`, all blocks in outer scope are handled properly and popped off

![pos9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos9.png)

in `position 10`, the right frame object is deallocated(actually it will become "zombie frame" of the code t object), the left frame object is in it's first try block

![pos10](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos10.png)