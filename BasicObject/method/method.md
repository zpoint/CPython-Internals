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

There's a type named **builtin_function_or_method** in Python. As the type name describes, all built-in functions or methods defined at the C level belong to **builtin_function_or_method**

```python3
>>> print
<built-in function print>
>>> type(print)
<class 'builtin_function_or_method'>

```

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/layout.png)

# example

## print

Let's read a code snippet first

```c
#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

```

**PyCFunction** is a type in C. Any C function with the signature (accepting two PyObject* as parameters and returning a PyObject*) can be cast to type **PyCFunction**

```c
// a c function named builtin_print
static PyObject *
builtin_print(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

// name "print" is defined inside a c array named builtin_methods, and defined s type PyMethodDef
static PyMethodDef builtin_methods[] = {
...
{"print",           (PyCFunction)(void(*)(void))builtin_print,      METH_FASTCALL | METH_KEYWORDS, print_doc},
...
}

```

![print](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print.png)

A **PyMethodDef** should be attached to a **module** object with a **self** argument, and then a **PyCFunctionObject** will be generated.

What the user actually interacts with is **PyCFunctionObject**

![print2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print2.png)

Let's check each field for more detail.

The type in the **m_self** field is **module**, and the type in the **m_module** field is **str**

![print3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print3.png)

# fields in PyMethodDef

## ml_name

As you can see in the above picture, the field **ml_name** is the name of the built-in method; it's a C-style null-terminated string "print"

## ml_meth

The actual C function pointer that does the job

## ml_flags

A bit flag that indicates how the C function behaves at the Python level.

The functions in **call.c** beginning with **_PyMethodDef_** will delegate work to the **PyCFunction**, but with different calling behavior according to different **ml_flags**

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

```c
static PyCFunctionObject *free_list = NULL;
static int numfree = 0;
#ifndef PyCFunction_MAXFREELIST
#define PyCFunction_MAXFREELIST 256
#endif

```

CPython uses a buffer pool with size 256 to reuse deallocated **PyCFunctionObject** objects. free_list is a singly linked list; all elements are chained by the **m_self** field.

A similar technique appears in float objects. Float objects are chained through the **ob_type** field. I will not draw again; users who need the graphical representation please click the link [float(free_list)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float.md#free_list)

# classmethod

# staticmethod

They're more related to **classobject**. I will talk about them later in [class object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class.md)