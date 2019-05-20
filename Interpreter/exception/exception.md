# exception

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
	* [base_exception](#base_exception)
* [exception handling](#exception-handling)

#### related file

* cpython/Include/cpython/pyerrors.h
* cpython/Objects/exceptions.c
* cpython/Lib/test/exception_hierarchy.txt

#### memory layout

##### base_exception

there are various exception types defined in `Include/cpython/pyerrors.h`, the most widely used **PyBaseExceptionObject**(also the base part of any other exception type)

![base_exception](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/base_exception.png)

all other basic exception types defined in same C file are shown

![error_layout1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout1.png)

![error_layout2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/error_layout2.png)

after define the basic exception types above, the following marco is used for defining all other exceptions according to the exception hierarchy

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
