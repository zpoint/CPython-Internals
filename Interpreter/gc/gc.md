# gc

### contents

* [related file](#related-file)
* [introduction](#introduction)
	* [reference counting](#reference-counting)
	* [generational garbage collection](#generational-garbage-collection)

#### related file

* cpython/Include/object.h
* cpython/Modules/gcmodule.c

#### introduction

the garbage collection in CPython consists of two components

* **reference counting**(mostly defined in `Include/object.h`)
* **generational garbage collection**(mostly defined in `Modules/gcmodule.c`)

##### reference counting

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

`4 LOAD_NAME                0` gets the value stores in local namespace with key 's' ---- the newly created, increment it's reference count, and push it onto the stack

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
        	/* // finds the descructor
            destructor dealloc = Py_TYPE(op)->tp_dealloc;
            calls the founded destructor
            (*dealloc)(op);
            */
            _Py_Dealloc(op);
        }
    }

##### generational garbage collection

