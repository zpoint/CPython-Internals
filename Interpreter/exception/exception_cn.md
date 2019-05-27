# exception

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [异常处理机制](#异常处理机制)

#### 相关位置文件

* cpython/Include/cpython/pyerrors.h
* cpython/Include/pyerrors.h
* cpython/Objects/exceptions.c
* cpython/Lib/test/exception_hierarchy.txt

#### 内存构造

在 `Include/cpython/pyerrors.h` 中定义了好几种异常类型, 应用范围最广的就是 **PyBaseExceptionObject** (同时也是所有异常类型都共有的基础部分)

![base_exception](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/base_exception.png)

其他基本的异常类型也定义在了同个 c 文件中

![error_layout1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout1.png)

![error_layout2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout2.png)

在定义了基础的异常类型之后, 还需要根据异常层级定义一些异常的继承类, 但是继承类有数十个, 这里用了几个 **marco** 来减小代码量

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

你可以在 `Lib/test/exception_hierarchy.txt` 这个文件里找到下面的层级

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

##### 异常处理机制

我们来定义一个示例看看

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


用 **dis** 模块处理它(有点长, 需要点耐心)

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


我们可以发现 opcode `0 SETUP_FINALLY          178 (to 202)` 对应到了函数 t 中最外层的 `finally`,
这里面 0 是当前 opcode 距离当前 code block 中第一个 opcode 的字节距离, 178 是 `SETUP_FINALLY` 这个 opcode 的参数, 真正异常发生时需要跳转到的 handler 的位置计算方式如下: `INSTR_OFFSET() + oparg`(`INSTR_OFFSET` 是下一个执行的 opcode 到头部 opcode 的距离(这里是 24), `oparg` 就是参数了, 这里是 178) 加起来的结果为 202

`2 SETUP_FINALLY           26 (to 52)` 对应到了函数 t 中最外层的 `except`, opcode 距离头部的字节距离为 26(`LOAD_GLOBAL`), 并且 `SETUP_FINALLY` 的参数也为 26, 加起来 26(参数) + 26(opcode 字节距离) 为 52

`SETUP_FINALLY` 干了什么 ?

    /* cpython/Python/ceval.c
    case TARGET(SETUP_FINALLY): {
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

对 frame 对象感兴趣的同学, 请参考 [frame 对象](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame_cn.md)

`PyFrame_BlockSetup` 会在 opcode `SETUP_FINALLY`, `SETUP_ASYNC_WITH` 或者 `SETUP_WITH` 中被调用 (以上三个 opcode 调用时 `int type` 的值为 `SETUP_FINALLY`)

也会在 `RAISE_VARARGS`, `END_FINALLY` 和 `END_ASYNC_FOR` 的某些分支下调用到

我们调用函数 `t2()` 的时候会发生什么 ?

这是 `PyTryBlock` 的定义


![try_block](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/try_block.png)

在 `position 1` 的位置

`CO_MAXBLOCKS` 的值为 20, 你不能在一个函数中定义超过 20 个 block (`try/finally/with/async with`)

不是说你能写 20 个 `try/except`, 一个 `try/except/finally` 可能对应不止一个 block)

![pos1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos1.png)

在 `position 2` 的位置

`STACK_LEVEL` 的定义为 `#define STACK_LEVEL()     ((int)(stack_pointer - f->f_valuestack))`, 他是在当前 frame 中, 当前 `stack_pointer` 的位置到 `f_valuestack` 的字节位置距离

![pos2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos2.png)

在 `position 3` 的位置

![pos3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos3.png)

在 `position 4`, 和 `t` 相关联的的 frame 对象现在有两个 `PyTryBlock`, 一个对应了最外层的 `finally` (opcode 位置 202), 另一个对应最外层的 `except` (opcode 位置 52)

![pos4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos4.png)

在 `position 5` 的位置, 第二个 `PyTryBlock` 中的 `b_type` 值为 `EXCEPT_HANDLER`(257), `b_handler` 变成了 -1

第二个 `PyTryBlock` 中的值是在执行这个 opcode `BINARY_TRUE_DIVIDE` 的过程中发生变化的

    case TARGET(BINARY_TRUE_DIVIDE): {
        PyObject *divisor = POP();
        PyObject *dividend = TOP();
        PyObject *quotient = PyNumber_TrueDivide(dividend, divisor);
        Py_DECREF(dividend);
        Py_DECREF(divisor);
        SET_TOP(quotient);
        if (quotient == NULL) // 0 不能做除数, 发生异常, 进到这里
            goto error;
        DISPATCH();
    }
    ...
    error:
    ...
    exception_unwind:
        /* 如果异常发生, 进到这里 */
        while (f->f_iblock > 0) {
            /* 取出当前的 block */
            PyTryBlock *b = &f->f_blockstack[--f->f_iblock];

            if (b->b_type == EXCEPT_HANDLER) {
            /* 如果这个异常已经进行过处理, 则不用再处理一遍 */
                UNWIND_EXCEPT_HANDLER(b);
                continue;
            }
            UNWIND_BLOCK(b);
            if (b->b_type == SETUP_FINALLY) {
            	/* 开始异常处理 */
                PyObject *exc, *val, *tb;
                int handler = b->b_handler;
                _PyErr_StackItem *exc_info = tstate->exc_info;
                /* 注意下面这个函数会让所有 b->b_* 字段失效 */
                /* 把这个 block 的这些字段改成标记值, 表示已经处理过, 这个异常就不会再次被处理 */
                /* b_type: EXCEPT_HANDLER(257), b_handler: -1, STACK_LEVEL: (int)(stack_pointer - f->f_valuestack)) */
                PyFrame_BlockSetup(f, EXCEPT_HANDLER, -1, STACK_LEVEL());
                ...
                /* set up some state */
                goto main_loop;
            }

看上面 **dis** 出的 opcode, `66 SETUP_FINALLY          116 (to 184)` 在 `print("position 5\n\n")` 之前, 所以这里会存在第三个 try block

这个 try block 是在编译过程产生的, opcode `184 LOAD_CONST               0 (None)` 的位置在 `position 8` 和 `position 9` 之间, 这几行 opcode 处理加载变量 `e1` 并释放之外没有做其他事情

![pos5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos5.png)

在 `position 6` 的位置, 下面的 opcode 又创建多了两个 try block

     12          82 SETUP_FINALLY           80 (to 164)
                 84 SETUP_FINALLY           26 (to 112)

![pos6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos6.png)

在 `position 7` 的位置, `104 IMPORT_NAME              5 (no)` 这里会抛出异常, 此时把最后一个 try block 标记为已处理的状态, 并跳转到对应的位置进行异常处理(`15     >>  112 DUP_TOP`), 从对应的位置执行到 `126 SETUP_FINALLY           18 (to 146)` 这里时, 又为变量 `e2` 创建了另一个 block

![pos7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos7.png)

在 `position 8` 的位置, 所有内嵌的 block 都处理完并且清除了

![pos8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos8.png)

在 `position 9` 的位置, 所有外层的 block 都处理完并且清除了

![pos9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos9.png)

在 `position 10` 的位置, 图片右边的 frame 对象进入回收流程(实际上他会变成 code t 对象的 "zombie frame"), 左边的 frame 对象正在它的第一个 try block 的处理位置中

![pos10](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/pos10.png)