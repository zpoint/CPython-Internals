# method

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
    * [print](#print)
* [fields in PyMethodDef](#fields-in-PyMethodDef)
* [free_list](#free_list)
* [classmethod](#classmethod)
* [staticmethod](#staticmethod)

# related file
* cpython/Objects/methodobject.c
* cpython/Include/methodobject.h
* cpython/Python/bltinmodule.c
* cpython/Objects/call.c

# memory layout

There's a type named **builtin_function_or_method** in python, as the type name described, all the builtin function or method defined in the c level belong to **builtin_function_or_method**

    >>> print
    <built-in function print>
    >>> type(print)
    <class 'builtin_function_or_method'>

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/layout.png)

# example

## print

let's read code snippet first

    #define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

    typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
    typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
    typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                                 PyObject *);
    typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                       PyObject *const *, Py_ssize_t,
                                                       PyObject *);
    typedef PyObject *(*PyNoArgsFunction)(PyObject *);

The **PyCFunction** is a type in c, any c function with signature(accept two PyObject* as parameters and return a PyObject *)  can be cast to type **PyCFunction**

    // a c function named builtin_print
    static PyObject *
    builtin_print(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

    // name "print" is defined inside a c array named builtin_methods, and defined s type PyMethodDef
    static PyMethodDef builtin_methods[] = {
    ...
    {"print",           (PyCFunction)(void(*)(void))builtin_print,      METH_FASTCALL | METH_KEYWORDS, print_doc},
    ...
    }

![print](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print.png)

a **PyMethodDef** should be attached to a **module** object, and a **self** arugment, and then a **PyCFunctionObject** will be generated

what user really interactive with is **PyCFunctionObject**

![print2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print2.png)

let's check for more detail of each field

the type in **m_self** field is **module**, and type in **m_module** field is **str**

![print3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print3.png)

# fields in PyMethodDef

## ml_name

as you can see in the above picture, field **ml_name** is the name of the built-in method, it's a c style null terminated string "print"

## ml_meth

the real c function pointer that does the job

## ml_flags

bit flag indicates how the c function's behave in the python level

the function in **call.c** begin with **_PyMethodDef_** will delegate the work to the **PyCFunction**, but with different calling behaviour according to different **ml_flags**

for more detail please refer to [c-api Common Object Structures](https://docs.python.org/3/c-api/structures.html)

| flag name | flag value | meaning |
| - | :-: | -: |
| METH_VARARGS | 0x0001| methods type PyCFunction, expects two PyObject* values |
| METH_KEYWORDS | 0x0002 | methods type PyCFunctionWithKeywords, expects three parameters: self, args, and a dictionary of all the keyword arguments |
| METH_NOARGS | 0x0004 | methods without parameters, need to be of type PyCFunction |
| METH_O | 0x0008 | methods with a single object argument, have the type PyCFunction |
| METH_CLASS | 0x0010 | the type object will be passed as first parameter, what @classmethod do |
| METH_STATIC | 0x0020 | null will passed as first parameter, what @staticmethod do |
| METH_COEXIST | 0x0040 | replace existing defination instead of skip |

# free_list

    static PyCFunctionObject *free_list = NULL;
    static int numfree = 0;
    #ifndef PyCFunction_MAXFREELIST
    #define PyCFunction_MAXFREELIST 256
    #endif

cpython use a buffer pool with size 256 to reuse the deallocated **PyCFunctionObject** object, free_list is a single linked list, all elements are chained by the **m_self** field

the similar technique appears in float object, the float object chained through the **ob_type** field, I will not draw again, user who need the graph representation please click the link [float(free_list)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float.md#free_list)

# classmethod

# staticmethod

they're more related in **classobject**, I will talk about them later in [class object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class.md)