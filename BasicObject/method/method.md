# method

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [print](#print)

### related file
* cpython/Objects/methodobject.c
* cpython/Include/methodobject.h
* cpython/Python/bltinmodule.c

#### memory layout

There's a type named **builtin_function_or_method** in python, as the type name described, all the builtin function or method defined in the c level belong to **builtin_function_or_method**

	>>> print
    <built-in function print>
    >>> type(print)
    <class 'builtin_function_or_method'>

![layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/method/layout.png)

#### example

##### print

let's look at some code snippet first

    #define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

    typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
    typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
    typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                                 PyObject *);
    typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                       PyObject *const *, Py_ssize_t,
                                                       PyObject *);
    typedef PyObject *(*PyNoArgsFunction)(PyObject *);

The **PyCFunction** is a type in c, any c function with signature(accept two **PyObject* ** as parameters and return a **PyObject * **)  can be cast to type **PyCFunction**

    // a c function named builtin_print
    static PyObject *
    builtin_print(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);
    // name "print" is defined inside a c array named builtin_methods, and defined s type PyMethodDef
    static PyMethodDef builtin_methods[] = {
    ...
    {"print",           (PyCFunction)(void(*)(void))builtin_print,      METH_FASTCALL | METH_KEYWORDS, print_doc},
    ...
    }

![print](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/method/print.png)

