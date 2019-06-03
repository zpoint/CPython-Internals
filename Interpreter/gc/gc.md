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

one is **reference counting**(mostly defined in `Include/object.h`), the other is **generational garbage collection**(mostly defined in `Modules/gcmodule.c`)

##### reference counting

as [wikipedia](https://en.wikipedia.org/wiki/Reference_counting) said

> In computer science, reference counting is a technique of storing the number of references, pointers, or handles to a resource such as an object, block of memory, disk space or other resource. It may also refer, more specifically, to a garbage collection algorithm that uses these reference counts to deallocate objects which are no longer referenced

###### example

in python

	s = "hello world"
    >>> id(s)
    4458636112
    >>> sys.getrefcount(s)
    2 # one from s, one from parameter of sys.getrefcount
	s2 = s
    >>> id(s2)
    4458636112 # sam as id(s)
    >>> sys.getrefcount(s2)
    3 # one more reference from s2
	>>> del s
    >>> sys.getrefcount(s2)
    2 # work as expected

usually, the assignment of an immutable ojbect such as `str` object, a deep copy will be made(the assigned object should have a different id), the above string `"hello world"` is an exception because it's so simple that it's `intern` flag will be set and it will be stored in a global dictionary, act as singleton, for more detail please refer to [str object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str.md)

if the right side of the assignment expression is a mutable object, a shallow copy will be made, object in the left and right side will have same id, the effort of the assignment makes one more reference to the real object

	s = []
    y = s
    >>> sys.getrefcount(s)
    3

if we compile the script

    s = "hello world"
    s2 = s
    del s

it becomes

    1         0 LOAD_CONST               0 ('hello world')
              2 STORE_NAME               0 (s)

    2         4 LOAD_NAME                0 (s)
              6 STORE_NAME               1 (s2)

    3         8 DELETE_NAME              0 (s)
             10 LOAD_CONST               1 (None)
             12 RETURN_VALUE



##### generational garbage collection

