# type

# contents

* [related file](#related-file)
* [mro](#mro)
	* [before python2.3](#before python2.3)
	* [after python2.3](#after python2.3)
	* [difference bwtween python2 and python3](#difference bwtween python2 and python3)
* [creation of class](#creation-of-class)
* [slot](#slot)

# related file
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h


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

![mro_ hierarchy](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/mro_ hierarchy.png)

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

