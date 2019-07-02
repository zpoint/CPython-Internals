# type

# contents

* [related file](#related-file)
* [mro](#mro)
	* [before python2.3](#before-python2.3)
	* [after python2.3](#after-python2.3)
	* [difference bwtween python2 and python3](#difference-bwtween-python2-and-python3)
* [creation of class](#creation-of-class)
* [creation of instance](#creation-of-instance)
* [metaclass](#metaclass)

# related file
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h
* cpython/Python/bltinmodule.c


# mro

the [C3 superclass linearization](https://en.wikipedia.org/wiki/C3_linearization) is implemented in python for Method Resolution Order (MRO) after python2.3, prior to python2.3, the Method Resolution Order is a Depth-First, Left-to-Right algorithm

for those who are interested in detail, please refer to [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)

## before python2.3

let's define an example

	from __future__ import print_function

    class A(object):
        pass

    class B(object):
        def who(self):
            print("I am B")

    class C(object):
        pass

    class D(A, B):
        pass

    class E(B, C):
        def who(self):
            print("I am E")

    class F(D, E):
        pass

![mro_ hierarchy](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/mro_hierarchy.png)

the **mro** order implementation before python2.3 is a Depth-First, Left-Most algorithm

mro of **D** is computed as `DAB`

mro of **E** is computed as `EBC`

mro of **F** is computed as `FDABEC`

	# ./python2.2
	>>> f = F()
	>>> f.who() # B is in front of E
	I am B

## after python2.3

the **merge** part of the **C3** algorithm can be simply summarized as the following steps

* take the head of the first list
* if this head is not in the tail of any of the other lists, then add it to the linearization of C and remove it from the lists in the merge
* otherwise look at the head of the next list and take it, if it is a good head
* then repeat the operation until all the class are removed or it is impossible to find good heads

mro of **D** is computed as

	D + merge(L(A), L(B), AB)
    D + merge(A, B, AB)
    # A is the head of the next list, and not the tail of any other list, take A out, remove A in the lists in the merge
    D + A + merge(B, B)
    # the first element is the list is the head, not the tail, so B is taken
    D + A + B

mro of **E** is computed as

	E + merge(L(B), L(C), BC)
    E + merge(B, C, BC)
    E + B + merge(C, C)
    E + B + C

mro of **F** is computed as

	F + merge(L(D), L(E), BC)
    F + merge(DAB, EBC, DE)
    F + D + merge(AB, EBC, E)
    F + D + A + merge(B, EBC, E)
    # B is now in the tail of the second list, the head of the second list is "E"
    # tail is "BC", so B can't be taken, next head E will be taken
    F + D + A + E + merge(B, BC)
    F + D + A + E + B + C

	# ./python2.3
	>>> f = F()
	>>> f.who() # E is in front of B
	I am E

## difference bwtween python2 and python3

in the following code

	from __future__ import print_function

    class A:
        pass

    class B:
        def who(self):
            print("I am B")

    class C:
        pass

    class D(A, B):
        pass

    class E(B, C):
        def who(self):
            print("I am E")

    class F(D, E):
        pass

in python2, class `A`, `B`,`C`, `D`, and `E` are not inherted from the `object`, even after python2.3, objects not inherted from `object` will not use the **C3** algorithm for **MRO**, the old style Depth-First, Left-Most algorithm will be used

	# ./python2.7
	>>> f = F()
	>>> f.who() # B is in front of E
	I am B


while in python3, they both implicit inherted from `object`, objects inherted from `object` use the **C3** algorithm for **MRO**

	# ./python3
	>>> f = F()
	>>> f.who() # E is in front of B
	I am E

# creation of class

if we `dis` the above code

     26          96 LOAD_BUILD_CLASS
                 98 LOAD_CONST              12 (<code object F at 0x10edf4150, file "mro.py", line 26>)
                100 LOAD_CONST              13 ('F')
                102 MAKE_FUNCTION            0
                104 LOAD_CONST              13 ('F')
                106 LOAD_NAME                6 (D)
                108 LOAD_NAME                7 (E)
                110 CALL_FUNCTION            4
                112 STORE_NAME               8 (F)

`LOAD_BUILD_CLASS` simply pushes the function `__build_class__` to stack

and the following opcodes push all the variables `__build_class__` needed to stack

`110 CALL_FUNCTION            4` calls the `__build_class__` to generate the class

the `__build_class__`  will find the `metaclass`, `name`, `bases`, and `ns`(namespace) and delegates the call to `metaclass.__call__` attribute

for example, the `class F`

	metaclass: <class 'type'>
	name: 'F'
    bases: (<class '__main__.D'>, <class '__main__.E'>)
    ns: {'__module__': '__main__', '__qualname__': 'F'}, cls: <class '__main__.F'>

in the example `class F`, what does `<class 'type'>.__call__` do ?

the function is defined in `cpython/Objects/typeobject.c`

prototype is `static PyObject *type_call(PyTypeObject *type, PyObject *args, PyObject *kwds)`

we can draw the following procedure according to the source code

![creation_of_class](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_class.png)

# creation of instance

if we add the following line to the tail of the above codes

	f = F()

we can find other snippet of the `dis` result


     31         130 LOAD_NAME                9 (F)
                132 CALL_FUNCTION            0
                134 STORE_NAME              10 (f)

it simply calls the `__call__` attribute of `F`


    >>> F.__call__
    <method-wrapper '__call__' of type object at 0x7fa5fa725ec8>

![creation_of_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_instance.png)

# metaclass

in the above procedures, we can learn that **metaclass** controls the creation of a **class**, the creation of **instance** doesn't invoke the **metaclass**, it just calls the `__call__` attribute of the class to create the instance

![difference_between_class_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/difference_between_class_instance.png)

we can change the behaviour of a class by manually define a **metaclass**


    class MyMeta(type):
        def __new__(mcs, name, bases, attrs, **kwargs):
            if F in bases:
                return F

            newly_created_cls = super().__new__(mcs, name, bases, attrs)
            if "Animal" in name:
                newly_created_cls.leg = 4
            else:
                newly_created_cls.leg = 0
            return newly_created_cls


    class Animal1(metaclass=MyMeta):
        pass


    class Normal(metaclass=MyMeta):
        pass


    class AnotherF(F, metaclass=MyMeta):
        pass


    print(Animal1.leg) # 4
    print(Normal.leg)  # 0
    print(AnotherF)  # <class '__main__.F'>

pretty fun!

# read more
* [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)
* [The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)