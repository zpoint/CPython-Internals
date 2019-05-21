# exception

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
    * [base_exception](#base_exception)
* [exception handling](#exception-handling)

#### related file

* cpython/Include/cpython/pyerrors.h
* cpython/Include/pyerrors.h
* cpython/Objects/exceptions.c
* cpython/Lib/test/exception_hierarchy.txt

#### memory layout

##### base_exception

there are various exception types defined in `Include/cpython/pyerrors.h`, the most widely used **PyBaseExceptionObject**(also the base part of any other exception type)

![base_exception](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/base_exception.png)

all other basic exception types defined in same C file are shown

![error_layout1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout1.png)

![error_layout2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout2.png)

what's following the definition of the basic exception types above are the definition of derived exception types according to exception hierarchy, because there're dozens of derived exceptions need to be defined, some **marco** are used for shortening the codes

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

you can find the exception hierarchy in `Lib/test/exception_hierarchy.txt`

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

##### exception handling

let's define an example

    def t():
        try:
            1 / 0
        except Exception as e1:
            try:
                import no
            except Exception as e2:
                pass
            finally:
                pass
        finally:
            pass

and try to compile it

    python.exe -m dis .\test.py
    1             0 LOAD_CONST               0 (<code object t at 0x05949BD0, file ".\test.py", line 1>)
                  2 LOAD_CONST               1 ('t')
                  4 MAKE_FUNCTION            0
                  6 STORE_NAME               0 (t)

     14           8 LOAD_NAME                0 (t)
                 10 CALL_FUNCTION            0
                 12 POP_TOP
                 14 LOAD_CONST               2 (None)
                 16 RETURN_VALUE

    Disassembly of <code object t at 0x05949BD0, file ".\test.py", line 1>:
      2           0 SETUP_FINALLY          108 (to 110)
                  2 SETUP_FINALLY           12 (to 16)

      3           4 LOAD_CONST               1 (1)
                  6 LOAD_CONST               2 (0)
                  8 BINARY_TRUE_DIVIDE
                 10 POP_TOP
                 12 POP_BLOCK
                 14 JUMP_FORWARD            90 (to 106)

      4     >>   16 DUP_TOP
                 18 LOAD_GLOBAL              0 (Exception)
                 20 COMPARE_OP              10 (exception match)
                 22 POP_JUMP_IF_FALSE      104
                 24 POP_TOP
                 26 STORE_FAST               0 (e1)
                 28 POP_TOP
                 30 SETUP_FINALLY           60 (to 92)

      5          32 SETUP_FINALLY           52 (to 86)
                 34 SETUP_FINALLY           12 (to 48)

      6          36 LOAD_CONST               2 (0)
                 38 LOAD_CONST               0 (None)
                 40 IMPORT_NAME              1 (no)
                 42 STORE_FAST               1 (no)
                 44 POP_BLOCK
                 46 JUMP_FORWARD            34 (to 82)

      7     >>   48 DUP_TOP
                 50 LOAD_GLOBAL              0 (Exception)
                 52 COMPARE_OP              10 (exception match)
                 54 POP_JUMP_IF_FALSE       80
                 56 POP_TOP
                 58 STORE_FAST               2 (e2)
                 60 POP_TOP
                 62 SETUP_FINALLY            4 (to 68)

      8          64 POP_BLOCK
                 66 BEGIN_FINALLY
            >>   68 LOAD_CONST               0 (None)
                 70 STORE_FAST               2 (e2)
                 72 DELETE_FAST              2 (e2)
                 74 END_FINALLY
                 76 POP_EXCEPT
                 78 JUMP_FORWARD             2 (to 82)
            >>   80 END_FINALLY
            >>   82 POP_BLOCK
                 84 BEGIN_FINALLY

     10     >>   86 END_FINALLY
                 88 POP_BLOCK
                 90 BEGIN_FINALLY
            >>   92 LOAD_CONST               0 (None)
                 94 STORE_FAST               0 (e1)
                 96 DELETE_FAST              0 (e1)
                 98 END_FINALLY
                100 POP_EXCEPT
                102 JUMP_FORWARD             2 (to 106)
            >>  104 END_FINALLY
            >>  106 POP_BLOCK
                108 BEGIN_FINALLY

     12     >>  110 END_FINALLY
                112 LOAD_CONST               0 (None)
                114 RETURN_VALUE

we can see that the opcode `0 SETUP_FINALLY          108 (to 110)` maps to the outermost `finally` statement, 0 is the byte offset of opcode, 108 is the parameter of the `SETUP_FINALLY`, the real handler offset is calculated as `INSTR_OFFSET() + oparg`(`INSTR_OFFSET` is the byte offset of the first opcode to the next opcode, `oparg` is the parameter 108), which is 110 in result

`2 SETUP_FINALLY           12 (to 16)` maps to the first `except` statement, 2 is the byte offset of opcode, 12 is the parameter of the `SETUP_FINALLY`, 12(parameter) + 4(opcode offset) is 16

what does `SETUP_FINALLY` do ?

    /* cpython/Python/ceval.c
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

for those who need detail of frame object, please refer to [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)