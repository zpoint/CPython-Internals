# gc

## contents

* [related file](#related-file)
* [introduction](#introduction)
	* [reference counting](#reference-counting)
		* [reference cycle problem](#reference-cycle-problem)
			* [example1](#example1)
			* [example2](#example2)
	* [generational garbage collection](#generational-garbage-collection)
		* [track](#track)
		* [generational](#generational)
		* [update_refs](#update_refs)
		* [subtract_refs](#subtract_refs)
		* [move_unreachable](#move_unreachable)
		* [finalize](#finalize)
		* [threshold](#threshold)
* [summary](#summary)

### related file

* cpython/Include/object.h
* cpython/Modules/gcmodule.c
* cpython/Include/internal/pycore_pymem.h

### introduction

the garbage collection in CPython consists of two components

* **reference counting** (mostly defined in `Include/object.h`)
* **generational garbage collection** (mostly defined in `Modules/gcmodule.c`)

#### reference counting

as [wikipedia](https://en.wikipedia.org/wiki/Reference_counting) says

> In computer science, reference counting is a technique of storing the number of references, pointers, or handles to a resource such as an object, block of memory, disk space or other resource. It may also refer, more specifically, to a garbage collection algorithm that uses these reference counts to deallocate objects which are no longer referenced

in python

	s = "hello world"

    >>> id(s)
    4346803024
    >>> sys.getrefcount(s)
    2 # one from s, one from parameter of sys.getrefcount

![refcount1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount1.png)

	>>> s2 = s
	>>> id(s2)
	4321629008 # sam as id(s)
    >>> sys.getrefcount(s)
    3 # one more from s2

![refcount2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount2.png)


let's compile a script

    s = []
    s2 = s
    del s

the output is

    1         0 BUILD_LIST               0
              2 STORE_NAME               0 (s)

    2         4 LOAD_NAME                0 (s)
              6 STORE_NAME               1 (s2)

    3         8 DELETE_NAME              0 (s)
             10 LOAD_CONST               0 (None)
             12 RETURN_VALUE

in the first line `s = []`

`0 BUILD_LIST               0` simply create a new list, initialize it's reference count to 1, and push it onto the stack

`2 STORE_NAME               0`

    case TARGET(STORE_NAME): {
        /* name is str object with value 's'
           ns is the local namespace
           v is the previous created list object
        */
        PyObject *name = GETITEM(names, oparg);
        PyObject *v = POP();
        PyObject *ns = f->f_locals;
        int err;
        if (ns == NULL) {
            PyErr_Format(PyExc_SystemError,
                         "no locals found when storing %R", name);
            Py_DECREF(v);
            goto error;
        }
        if (PyDict_CheckExact(ns))
        	/* here, reference count of v is 1
        	   PyDict_SetItem will add 's' to the local namespace with value 'v'
               since ns is of type dict, the add operation will increment the reference count of 's' and 'v'
           */
            err = PyDict_SetItem(ns, name, v);
            /* after add, reference count of v becomes 2 */
        else
            err = PyObject_SetItem(ns, name, v);
        Py_DECREF(v);
        /* after Py_DECREF, reference count of v becomes 1 */
        if (err != 0)
            goto error;
        DISPATCH();
    }

before execute line 2 `s2 = s`, reference count of the newly created list object is 1

`4 LOAD_NAME                0` gets the value stores in local namespace with key 's' ---- the newly created list object, increment it's reference count, and push it onto the stack

until now, the reference count of the newly created list object is 2(one from 's', one from the stack)

`6 STORE_NAME               1` executes the above code again, the difference is the name now becomes 's2', after this opcode, reference count of the list becomes 2

`8 DELETE_NAME              0 (s)`

    case TARGET(DELETE_NAME): {
    	/* name here is 's'
           ns is the local namespace
        */
        PyObject *name = GETITEM(names, oparg);
        PyObject *ns = f->f_locals;
        int err;
        if (ns == NULL) {
            PyErr_Format(PyExc_SystemError,
                         "no locals when deleting %R", name);
            goto error;
        }
        /* until now, reference count of the list object is 2
        /* find the location with key 's', mark the indices as DKIX_DUMMY,
        set the entries key and value to NULL, decrement the reference count of key and value
        */
        err = PyObject_DelItem(ns, name);
        /* reach here, reference count of the list object becomes 1 */
        if (err != 0) {
            format_exc_check_arg(PyExc_NameError,
                                 NAME_ERROR_MSG,
                                 name);
            goto error;
        }
        DISPATCH();
    }

if the reference count becomes 0, the deallocate procedure will be triggered imeediately

	/* cpython/Include/object.h */
    static inline void _Py_DECREF(const char *filename, int lineno,
                                  PyObject *op)
    {
        _Py_DEC_REFTOTAL;
        if (--op->ob_refcnt != 0) {
    #ifdef Py_REF_DEBUG
            if (op->ob_refcnt < 0) {
                _Py_NegativeRefcount(filename, lineno, op);
            }
    #endif
        }
        else {
        	/* // _Py_Dealloc will find the descructor, and calls the founded destructor
            destructor dealloc = Py_TYPE(op)->tp_dealloc;
            (*dealloc)(op);
            */
            _Py_Dealloc(op);
        }
    }

##### reference cycle problem

there're some situations that the reference counting can't handle

##### example1

    class A:
        pass

    >>> a1 = A()
    >>> a2 = A()
    >>> a1.other = a2
    >>> a2.other = a1

reference count of **a1** and **a2** are both 2

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each1.png)

    >>> del a1
    >>> del a2

now, the reference from local namespace is deleted, and they both have a reference to each other, there's no way for the reference count of **a1**/**a2** to become 0

![ref_cycle2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each2.png)

##### example2

	>>> a = list()
	>>> a.append(a)
	>>> a
	[[...]]

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle1.png)

	>>> del a

the reference from local namespace is deleted, the reference count of **a** will stays 1 and never become 0

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle2.png)

#### generational garbage collection

If there's only one mechanism **reference counting** working, the interpreter will risk the memory leak issue due to the reference cycle in the above examples

**generational garbage collection** is the other component used for detecting and garbage collecting the unreachable object

    class A:
        pass

    >>> a1 = A()
    >>> a2 = A()
    >>> a1.other = a2
    >>> a2.other = a1

    >>> b = list()
    >>> b.append(b)

##### track

there must be a way to keep track of all the heap allocated **PyObject**

**generation0** is a pointer, to the head of the linked list, each element in the linked list consists of two parts, the first part is **PyGC_Head**, the second part is **PyObject**

**PyGC_Head** contains a **_gc_next** pointer(for tracking the next element in linked list) and a **_gc_prev** pointer(for tracking the previous element in linked list)

**PyObject** is the base structure of a python object

![track](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/track.png)

when you create a python object such as `a = list()`, object **a** of type **list** will be appended to the end of the linked list in **generation0**, so **generation0** is able to track all the newly created python object

actually, the linked list is linked via **_gc_next**, **_gc_prev** is not a valid pointer, it's used for storing bit flag and other information

##### generational

if all newly created objects are appended to the tail of one linked list, somehow it will be a very huge linked list

for some program designed for running for a long time, there must exist some long lived objects, repeating garbage collecting these objects will waste lots of CPU time

generations is used in the purpose of doing less collections

Cpython used three generations totally, the newly created objects is stored in the first generation, when an object survive a round of gc, it will be moved to next generation

lower generation will be collected more frequently, higher generation will be collected less frequently

when a generation is being collected, all the generations lower than the current will be merged together before collection

![generation](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation.png)

##### update_refs

let's run a procedure of garbage collection

	# assume a1, a2, b survived in generation0, and move to generation1
    >>> c = list()
    >>> d1 = A()
    >>> d2 = A()
    >>> d1.other2 = d2
    >>> d2.other2 = d1

    >>> del a1
    >>> del a2
    >>> del b
    >>> del d1
	>>> gc.collect() # assume collect generation1

![update_ref1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref1.png)

first step is to merge every generation lower than the current generation, mark this merged generation as **young**, mark the generation next to the current generation as **old**

![young_old](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/young_old.png)

function **update_refs** will copy object's reference count, stores in the **_gc_prev** field of the object, the right most 2 bit of **_gc_prev** is reserved of some marking

so, the copy of the reference count will left shift 2 before stores in the **_gc_prev**

`1 / 0x06` means the copy of the reference count is `1`, but the value actually stores in **_gc_prev** is `0x06`

![update_ref2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref2.png)

##### subtract_refs

function **subtract_refs** will traverse the generation, iterate through every object in  the linked list

for every object in the generation, check if every other object accessible from the current object is collectable and in the same generation, if so, decrement the copied reference count in **_gc_prev** (calls **tp_traverse** with the function **visit_decref**)

this step aims to decrement reference count referenced by those objects in same generation

![subtract_refs](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/subtract_refs.png)

##### move_unreachable

**move_unreachable** will create a linked list named **unreachable**, traverse the current **generation**, move those objects with the value of the copy of the reference count <= 0 to **unreachable**

if the current object has the value of the copy of the reference count > 0, mark every other object accessible from the current object as reachable, if the accessible object is currently in **unreachable**, move it back to normal **generation**, and reset the **_gc_prev** pointer

first object in **generation** is the local **namespace**, because **namespace** object is reachable, all objects reachable from the local **namespace** is also reachable, so the **_gc_prev** of `c` and `d2` are all set to `0x06`

![move_unreachable1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable1.png)

`a1` and `a2` are moved to unreachable

![move_unreachable2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable2.png)

### summary

because the gc algorithm CPython used is not a parallel algorithm, a global lock such as [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md) is necessary to protect the critical region when set up the track of an object to gc, or when garbage collecting any of the generations

while other garbage collector in other programming language such as [Java-g1](http://idiotsky.top/2018/07/28/java-g1/) use **Young GC** or **Mix GC**(combined with Tri-Color algorithm for global concurrent marking) to do the garbage collection
