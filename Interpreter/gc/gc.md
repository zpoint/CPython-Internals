# gc![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/gc)

* you may need more than 15 minutes to read this article

# contents

* [related file](#related-file)
* [introduction](#introduction)
	* [reference counting](#reference-counting)
		* [when will it be triggered](#when-will-it-be-triggered)
		* [reference cycle problem](#reference-cycle-problem)
			* [example1](#example1)
			* [example2](#example2)
	* [generational garbage collection](#generational-garbage-collection)
		* [track](#track)
		* [generational](#generational)
		* [update_refs](#update_refs)
		* [subtract_refs](#subtract_refs)
		* [move_unreachable](#move_unreachable)
		* [finalizer](#finalizer)
		* [threshold](#threshold)
		* [when will generational gc be triggered](#when-will-generational-gc-be-triggered)
* [summary](#summary)
* [read more](#read-more)

# related file

* cpython/Include/object.h
* cpython/Modules/gcmodule.c
* cpython/Include/internal/pycore_pymem.h

# introduction

the garbage collection in CPython consists of two components

* **reference counting** (mostly defined in `Include/object.h`)
* **generational garbage collection** (mostly defined in `Modules/gcmodule.c`)

## reference counting

as [wikipedia](https://en.wikipedia.org/wiki/Reference_counting) says

> In computer science, reference counting is a technique of storing the number of references, pointers, or handles to a resource such as an object, block of memory, disk space or other resource. It may also refer, more specifically, to a garbage collection algorithm that uses these reference counts to deallocate objects which are no longer referenced

in python

```python3
s = "hello world"

>>> id(s)
4346803024
>>> sys.getrefcount(s)
2 # one from s, one from parameter of sys.getrefcount

```

![refcount1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount1.png)

```python3
>>> s2 = s
>>> id(s2)
4321629008 # sam as id(s)
>>> sys.getrefcount(s)
3 # one more from s2

```

![refcount2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount2.png)


let's compile a script

```python3
s = []
s2 = s
del s

```

the output is

```python3
1         0 BUILD_LIST               0
          2 STORE_NAME               0 (s)

2         4 LOAD_NAME                0 (s)
          6 STORE_NAME               1 (s2)

3         8 DELETE_NAME              0 (s)
         10 LOAD_CONST               0 (None)
         12 RETURN_VALUE

```

in the first line `s = []`

`0 BUILD_LIST               0` simply create a new list, initialize it's reference count to 1, and push it onto the stack

`2 STORE_NAME               0`

```c
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
    	   PyDict_SetItem will add 's' to the local namespace with value v(the newly created list)
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

```

before execute line 2 `s2 = s`, reference count of the newly created list object is 1

`4 LOAD_NAME                0` gets the value stores in local namespace with key 's' ---- the newly created list object, increment it's reference count, and push it onto the stack

until now, the reference count of the newly created list object is 2(one from 's', one from the stack)

`6 STORE_NAME               1` executes the above code again, the difference is the name now becomes 's2', after this opcode, reference count of the list becomes 2

`8 DELETE_NAME              0 (s)`

```c
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
       find the location with key 's', mark the indices as DKIX_DUMMY,
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

```

### when will it be triggered

if the reference count becomes 0, the deallocate procedure will be triggered immediately

```c
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

```

### reference cycle problem

there're some situations that the reference counting can't handle

### example1

```python3
class A:
    pass

>>> a1 = A()
>>> a2 = A()
>>> a1.other = a2
>>> a2.other = a1

```

reference count of **a1** and **a2** are both 2

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each1.png)

```python3
>>> del a1
>>> del a2

```

now, the reference from local namespace is deleted, and they both have a reference to each other, there's no way for the reference count of **a1**/**a2** to become 0

![ref_cycle2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_each2.png)

### example2

```python3
>>> a = list()
>>> a.append(a)
>>> a
[[...]]

```

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle1.png)

```python3
>>> del a

```

the reference from local namespace is deleted, the reference count of **a** will stays 1 and never become 0

![ref_cycle1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/ref_cycle2.png)

## generational garbage collection

If there's only one mechanism **reference counting** working, the interpreter will risk the memory leak issue due to the reference cycle in the above examples

**generational garbage collection** is the other component used for detecting and garbage collecting the unreachable object

```python3
class A:
    pass

>>> a1 = A()
>>> a2 = A()
>>> a1.other = a2
>>> a2.other = a1

>>> b = list()
>>> b.append(b)

```

### track

there must be a way to keep track of all the heap allocated **PyObject**

**generation0** is a pointer, to the head of the linked list, each element in the linked list consists of two parts, the first part is **PyGC_Head**, the second part is **PyObject**

**PyGC_Head** contains a **_gc_next** pointer(for tracking the next element in linked list) and a **_gc_prev** pointer(for tracking the previous element in linked list)

**PyObject** is the base structure of a python object

![track](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/track.png)

when you create a python object such as `a = list()`, object **a** of type **list** will be appended to the end of the linked list in **generation0**, so **generation0** is able to track all the newly created python object

actually, the linked list is linked via **_gc_next**, **_gc_prev** is not a valid pointer, it's used for storing bit flag and other information

### generational

if all newly created objects are appended to the tail of one linked list, somehow it will be a very huge linked list

for some program designed for running for a long time, there must exist some long lived objects, repeating garbage collecting these objects will waste lots of CPU time

generations is used in the purpose of doing less collections

CPython used three generations totally, the newly created objects is stored in the first generation, when an object survive a round of gc, it will be moved to next generation

lower generation will be collected more frequently, higher generation will be collected less frequently

when a generation is being collected, all the generations lower than the current will be merged together before collection

![generation](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation.png)

### update_refs

let's run a procedure of garbage collection

```python3
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
>>> gc.collect(1) # assume collect generation1

```

![update_ref1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref1.png)

first step is to merge every generation lower than the current generation, mark this merged generation as **young**, mark the generation next to the current generation as **old**

![young_old](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/young_old.png)

function **update_refs** will copy object's reference count, stores in the **_gc_prev** field of the object, the right most 2 bit of **_gc_prev** is reserved of some marking

so, the copy of the reference count will left shift 2 before stores in the **_gc_prev**

`1 / 0x06` means the copy of the reference count is `1`, but the value actually stores in **_gc_prev** is `0x06`

![update_ref2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/update_ref2.png)

### subtract_refs

function **subtract_refs** will traverse the **young** generation, iterate through every object in  the linked list

for every object in the generation, check if every other object accessible from the current object is collectable and in the same generation, if so, decrement the copied reference count in **_gc_prev** (calls **tp_traverse** with the function **visit_decref**)

this step aims to decrement reference count referenced by those objects in same generation

![subtract_refs](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/subtract_refs.png)

### move_unreachable

**move_unreachable** will create a linked list named **unreachable**, traverse the current **generation** named **young**, move those objects with the value of the copy of the reference count <= 0 to **unreachable**

if the current object has the value of the copy of the reference count > 0, for every other object in same generation reachable from the current object, if the accessible object has a copy of the reference count <= 0, reset the reference count to 1, and move that object to the tail of **generation** if it's currentlt in **unreachable**

let's see an example

first object in **generation** is the local **namespace**, because **namespace** object is reachable, all objects reachable from the local **namespace** is also reachable, so the **_gc_prev** of `c` and `d2` are all set to `0x06`

![move_unreachable1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable1.png)

the second object is `a1`, since `a1`'s copy of the reference count is <= 0, it's moved to **unreachable**

![move_unreachable2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable2.png)

so does `a2`

![move_unreachable3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable3.png)

`b`'s copy of reference count is also <= 0

![move_unreachable4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable4.png)

now, it comes to `c`

because the copy of the reference count of `c`'s value is > 0, it will stay in the **generation**

![move_unreachable5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable5.png)

because `d1`'s copy of the reference count is <= 0, it's moved to **unreachable**

![move_unreachable6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable6.png)

because `d2`'s copy of the reference count is > 0, and `d1` is reachable from `d2`

![move_unreachable7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable7.png)

`d1`'s copy of the reference count will be set to 1, and `d1` will be appended to the tail of the **generation**

![move_unreachable8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable8.png)

when it comes to `d1`, copy of the reference count is > 0

and we reach the end of the **generation**, so every object stays in **unreachable** can be garbage collected

all objects survive this round of garbage collections will be moved to the elder **generation**

![move_unreachable9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/move_unreachable9.png)

### finalizer

what if the object needed to be garbage collected has defined it's own finalizer ?

before python3.4, those objects won't be collected, they will be moved to `gc.garbage` and you need to call `__del__` and collect them manually

after python3.4, [pep-0442](https://legacy.python.org/dev/peps/pep-0442/) solves the problem

```python3
class A:
    pass


class B(list):
    def __del__(self):
        a3.append(self)
        print("del of B")


a1 = A()
a2 = A()
a1.other = a2
a2.other = a1


a3 = list()
b = B()
b.append(b)
del a1
del a2
del b
gc.collect()

```

this is the layout after **move_unreachable**

![finalize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize1.png)

step1, all objects defined it's own finalizer,  the `__del__` methods will be called

after `__del__`

![finalize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize2.png)

step2, do **update_refs** in **unreachable**

notice, the first bit flag of `b` in **_gc_prev** is set, indicate that it's finalizer is called

![finalize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize3.png)

step3, do **subtract_refs** in **unreachable**

this step aims to elimate the reference cycle among objects in **unreachable**

then, traverse every object in **unreachable**, check if any object with the value of copy of the reference count > 0, if so, it means at least one object in **unreachable** makes outer object create new reference to this object, then go to step4

if not, after traverse, collecte all objects in **unreachable** and return

![finalize4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize4.png)

stp4, move all objects in **unreachable** to **old** generation

if there's any object in **unreachable** defined it's own `__del__`, the `__del__` methods will be called in step1, and all objects in **unreachable** will survive this round of garbage collection

![finalize5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/finalize5.png)

in the next round of gc, object `a1` and `a2` will be collected, because object `b`'s reference count is greater than 0 after the **subtract_refs**, it won't be moved to **unreachable**

if the `__del__` methods is called, the bit flag in **_gc_prev** will be set, so the `__del__` will be called only once

### threshold

there are three generations totally, and three **threshold**, each generation has a **threshold**

before performing collection, CPython will exam from high generation to low generation, if currently the number of objects in the current generation is greater than **threshold**, **gc** will begin in the current generation

```python3
>>> gc.get_threshold()
(700, 10, 10) # default value

```

it can be set manually

```python3
gc.set_threshold(500)
gc.set_threshold(100, 20)

```

### when will generational gc be triggered

one way is to call `gc.collect()` manually, it will collect the highest collection directly

```python3
>>> imporr gc
>>> gc.collect()

```

the other way is when the intepreter malloc a new space from the heap

![generation_trigger1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger1.png)

the `collect` procedure will check from the highest to lowest generation

first check the **generation2**, **generation2**'s count is less than **threashold**

![generation_trigger2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger2.png)

then check **generation1**, **generation1**'s count is greater than **threashold**, collect procedure begins in **generation1**

![generation_trigger3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger3.png)

the collect procedure will merge all the generations lower than the current, and do what's described above

![generation_trigger4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/generation_trigger4.png)

if the gc is collecting the highest generation, all the free_list will be freed, if you read other article such as [list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list.md) or [tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md), you can find what's free_list

# summary

because the gc algorithm CPython used is not a parallel algorithm, a global lock such as [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md) is necessary to protect the critical region when set up the track of an object to gc, or when garbage collecting any of the generations

while other garbage collector in other programming language such as [Java-g1](http://idiotsky.top/2018/07/28/java-g1/) use **Young GC** or **Mix GC**(combined with Tri-Color algorithm for global concurrent marking) to do the garbage collection


# read more

* [Garbage collection in Python: things you need to know](https://rushter.com/blog/python-garbage-collector/)
* [the garbage collector](https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/)