# memory management

## contents

* [related file](#related-file)
* [introduction](#introduction)
	* [object allocator](#object-allocator)
	* [raw memory allocator](#raw-memory-allocator)
	* [block](#block)
	* [pool](#pool)
	* [arena](#arena)
	* [memory layout](#memory-layout)
	* [usedpools](#usedpools)


## related file

* cpython/Modules/gcmodule.c
* cpython/Objects/object.c
* cpython/Include/cpython/object.h
* cpython/Include/object.h
* cpython/Include/objimpl.h
* cpython/Objects/obmalloc.c

## introduction

CPython has defined it's own memory management mechanism, when you create a new object in python program, it's not directly malloced from the heap

![level](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/level.png)

we will figure out how the **python interpreter** part works internally in the following example

### object allocator

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

### raw memory allocator

follow the call stack, we can find that the **raw memory allocator** is mostly defined in `cpython/Objects/obmalloc.c`

	#define SMALL_REQUEST_THRESHOLD 512

the procedure is described below

* if the request memory block is greater than **SMALL_REQUEST_THRESHOLD**(512 bytes), the request will be delegated to the operating system's allocator
* else, the request will be delegated to python's **raw memory allocator**

### block

we need to know some concept before we look into how python's memory allocator work

**block** is the smallest unit in python's memory management system, the size of a blok is the same size as a single **byte**

	typedef uint8_t block

there're lots of memory **block** in differenct size, word **block** in memory **block** has a different meaning from type **block**, we will see later

### pool

a **pool** stores a collection of memory **block** of the same size

usually, the total size of memory blocks in a pool is 4kb, which is the same as system page size

initially, the addresses for different memory block in the same pool are continously

![pool](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool.png)

### arena

an **arena** stores 256kb malloced from heap, it divides the memory block to 4kb per unit

whenever there needs a new pool, the allocator will ask an **arena** for a new memory block with size 4kb, and initialize the memory block as a new **pool**

![arena](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena.png)

### memory layout

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/layout.png)

### usedpools

there's a C array named **usedpools** plays an important role in the memory management mechanism

the defination of **usedpools** in C is convoluted, the following picture shows how these mentioned objects organized

![usedpools](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/usedpools.png)

element in **usedpools** is of type `pool_header *`, size of **usedpools** is 128, but only half of the element is in use

	 cpython/Objects/obmalloc.c
     * For small requests we have the following table:
     *
     * Request in bytes     Size of allocated block      Size class idx
     * ----------------------------------------------------------------
     *        1-8                     8                       0
     *        9-16                   16                       1
     *       17-24                   24                       2
     *       25-32                   32                       3
     *       33-40                   40                       4
     *       41-48                   48                       5
     *       49-56                   56                       6
     *       57-64                   64                       7
     *       65-72                   72                       8
     *        ...                   ...                     ...
     *      497-504                 504                      62
     *      505-512                 512                      63
     *
     *      0, SMALL_REQUEST_THRESHOLD + 1 and up: routed to the underlying
     *      allocator.
     */

**idx0** is the head of a double linked list, each element in the double linked list is a pointer to a **pool**, all pools in **idx0** will handle those memory request <= 8 byte, no matter how many bytes caller request, **pool** in **idx0** will only return a memory block of size 8 bytes each time

**idx2** is the head of a double linked list, all pools in **idx0** will handle those memory request (9 bytes <= request_size <= 16 bytes), no matter how many bytes caller request, **pool** in **idx0** will only return a memory block of size 16 bytes each time

and so on
