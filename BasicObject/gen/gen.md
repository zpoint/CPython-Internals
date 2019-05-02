# gen

### category

* [related file](#related-file)
* [generator](#generator)
	* [memory layout](#memory-layout-generator)
	* [example generator](#example-generator)
* [coroutine](#coroutine)
	* [example coroutine](#example-coroutine)
* [async generator](#async-generator)
	* [example async generator](#example-async-generator)

### related file
* cpython/Objects/genobject.c
* cpython/Include/genobject.h

#### memory layout generator

there's a common defination among **generator**, **coroutine** and **async generator**

    #define _PyGenObject_HEAD(prefix)                                           \
        PyObject_HEAD                                                           \
        /* Note: gi_frame can be NULL if the generator is "finished" */         \
        struct _frame *prefix##_frame;                                          \
        /* True if generator is being executed. */                              \
        char prefix##_running;                                                  \
        /* The code object backing the generator */                             \
        PyObject *prefix##_code;                                                \
        /* List of weak reference. */                                           \
        PyObject *prefix##_weakreflist;                                         \
        /* Name of the generator. */                                            \
        PyObject *prefix##_name;                                                \
        /* Qualified name of the generator. */                                  \
        PyObject *prefix##_qualname;                                            \
        _PyErr_StackItem prefix##_exc_state;

the defination of **generator** object is less than 4 lines

    typedef struct {
        /* The gi_ prefix is intended to remind of generator-iterator. */
        _PyGenObject_HEAD(gi)
    } PyGenObject;

which can be expanded to

    typedef struct {
        struct _frame *gi_frame;
        char gi_running;
        PyObject *gi_code;
        PyObject *gi_weakreflist;
        PyObject *gi_name;
        PyObject *gi_qualname;
        _PyErr_StackItem gi_exc_state;
    } PyGenObject;

we can draw the layout according to the code now

![layout_gen](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/gen/layout_gen.png)

#### example generator

