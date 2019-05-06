# set

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [fields](#fields)
	* [im_func](#im_func)
	* [im_self](#im_self)
* [free list](#free-list)

#### related file
* cpython/Objects/classobject.c
* cpython/Include/classobject.h

#### memory layout

the **PyMethodObject** represents the type **method** in c-level

    class C(object):
        def f1(self, val):
            return val

    >>> c = C()
    >>> type(c.f1)
    method

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/layout.png)

#### fields

##### im_func

##### im_self

#### free list
