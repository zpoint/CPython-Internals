# slot

# contents

* [related file](#related-file)
* [slot](#slot)
* [example](#example)
* [access instance](#access-instance)
	* [access wing](#access-wing)
	* [before set a value](#before-set-a-value)
	* [after set a value](#after-set-a-value)
	* [access x](#access-x)
* [access type](#access-type)
* [read more](#read-more)

# related file
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h


# slot

# example

    class A(object):
        __slots__ = ["wing", "leg"]

        x = 3


what's the difference of accessing attribute `wing` and `x` of instance `a` ?

what's the difference of accessing attribute `wing` and `x` of type `A` ?

# access instance

## access wing

### before set a value

	>>> a = A()
    >>> a.wing
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
    AttributeError: wing

according to the **attribute accessing procedure** described in [descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md)

we can draw the procedure of accessing `a.wing` in  higher level

![instance_desc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_desc.png)

the **descriptor protocol** will help us find an object named `descr`, type is `member_descriptor`, if you call

	repr(descr)
    descr: <member 'wing' of 'A' objects>

the `descr` has a member named `d_member`, which stores information about the attribute name, type and offset

with the informations stored in `d_member`, you are able to access the address of attribute in a very fast way

in the current example, if the begin address of the instance `a` is `0x00`, address after adding the offset is `0x18`, you can cast the address in `0x18` to a proper object according to `type` stored in `d_member`

	/* Include/structmember.h */
    #define T_OBJECT_EX 16  /* Like T_OBJECT, but raises AttributeError
                               when the value is NULL, instead of
                               converting to None. */

	/* Python/structmember.c */
    addr += l->offset;
    switch (l->type) {
    	/* ... */
        case T_OBJECT_EX:
            v = *(PyObject **)addr;
            /* because wing attribute of instance a is never set,
            it raise an attribute error */
            if (v == NULL)
                PyErr_SetString(PyExc_AttributeError, l->name);
            Py_XINCREF(v);
            break;
        /* ... */
    }

![offset](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/offset.png)

### after set a value

	>>> a = A()
    >>> a.wing = "wingA"
    >>> a.wing
    'wingA'

because wing attribute of instance `a` is set before, `AttributeError` won't be raised

## access x

	>>> a.x
	>>> 3

type of `descr` is `int`, it's not a data descriptor(does not have `__get__` or `__set__` attribute), so the `descr` will be returned directly

![instance_normal](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/instance_normal.png)

you are not able to create any other attribute in instance `a` if `A` defined `__slots__`, we will figure out why later

	>>> a.not_exist = 100
	Traceback (most recent call last):
	File "<input>", line 1, in <module>
	AttributeError: 'A' object has no attribute 'not_exist'

# access type



creation of `class A` ?

# read more
* [`__slots__`magic](http://book.pythontips.com/en/latest/__slots__magic.html)
* [A quick dive into Python’s `__slots__`](https://blog.usejournal.com/a-quick-dive-into-pythons-slots-72cdc2d334e?gi=26f20645d6c9)