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

    import sys

    def f(param):
    	print(id(param))
        print(sys.getrefcount(param))

	s = "hello world"


    >>> id(s)
    4321629008
    >>> sys.getrefcount(s)
    2 # one from s, one from parameter of sys.getrefcount

![refcount1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount1.png)

	>>> f(s)
	4321629008 # sam as id(s)
    4 # one more from param of f, one more from python's stack in frame

![refcount2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/refcount2.png)


if we compile the script

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




##### generational garbage collection

