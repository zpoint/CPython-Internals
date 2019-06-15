# memory management

### contents

* [related file](#related-file)
* [introduction](#introduction)
	* [object allocator](#object-allocator)
	* [raw memory allocator](#raw-memory-allocator)

### related file

* cpython/Modules/gcmodule.c
* cpython/Objects/object.c
* cpython/Include/cpython/object.h
* cpython/Include/object.h
* cpython/Include/objimpl.h
* cpython/Objects/obmalloc.c

### introduction

CPython has defined it's own memory management mechanism, when you create a new object in python program, it's not directly malloced from the heap

![level](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/level.png)

we will figure out how the **python interpreter** part works internally in the following example

#### object allocator

this is the created procedure of a `tuple` object with size n

step1, check if there's a chance to reuse `tuple` object in [free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#free-list), if so, goes to step4

step2, call **PyObject_GC_NewVar** to get a `PyTupleObject` with size `n` from memory management system(if the size of the object is fixed, **_PyObject_GC_New** will called instead)

step3, call **_PyObject_GC_TRACK** to link the newly created object to the double linked list in [gc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md#track).**generation0**

step4, return

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/tuple_new.png)

the maximum size of memory you are able to allocated is limited in **_PyObject_GC_Alloc**

	typedef ssize_t         Py_ssize_t;
	#define PY_SSIZE_T_MAX ((Py_ssize_t)(((size_t)-1)>>1))
    # PY_SSIZE_T_MAX is 8388608 TB in my machine, usually you don't need to worry about the limit
    if (basicsize > PY_SSIZE_T_MAX - sizeof(PyGC_Head))
        return PyErr_NoMemory();

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/malloc.png)

#### raw memory allocator

follow the calling stack, we can find that the **raw memory allocator** is mostly defined in `cpython/Objects/obmalloc.c`
