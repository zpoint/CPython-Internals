# slot

# contents

* [related file](#related-file)
* [slot](#slot)
* [example](#example)

# related file
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h


# slot

# example

    class A(object):
        __slots__ = ["wing", "leg"]

        x = 3



what's the difference of accessing attribute `wing` and `x` of type `A` ?

	to be continued

what's the difference of accessing attribute `wing` and `x` of instance `a` ?

	a = A()
    a.wing
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
    AttributeError: wing

according to the **attribute accessing procedure** described in [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md)

we can draw the procedure of accessing `a.wing` in  higher level



creation of `class A` ?

