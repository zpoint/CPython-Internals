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
* [read more](#read-more)

# related file
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h
* cpython/Python/bltinmodule.c


# mro

The [C3 superclass linearization](https://en.wikipedia.org/wiki/C3_linearization) is implemented in Python for Method Resolution Order (MRO) after Python 2.3. Prior to Python 2.3, the Method Resolution Order was a Depth-First, Left-to-Right algorithm

for those who are interested in detail, please refer to [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)

## before python2.3

Let's define an example

```python3
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

```

![mro_ hierarchy](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/mro_hierarchy.png)

The **mro** order implementation before Python 2.3 was a Depth-First, Left-Most algorithm

mro of **D** is computed as `DAB`

mro of **E** is computed as `EBC`

mro of **F** is computed as `FDABEC`

```python3
# ./python2.2
>>> f = F()
>>> f.who() # B is in front of E
I am B

```

## after python2.3

The **merge** part of the **C3** algorithm can be simply summarized as the following steps:

* Take the head of the first list
* If this head is not in the tail of any of the other lists, then add it to the linearization of C and remove it from the lists in the merge
* Otherwise, look at the head of the next list and take it if it is a good head
* Then repeat the operation until all the classes are removed or it is impossible to find good heads (the inherited order needs to be monotonous, or the algorithm won't terminate; for more detail please refer to the first video in [read more](#read-more))

mro of **D** is computed as

```python3
D + merge(L(A), L(B), AB)
D + merge(A, B, AB)
# A is the head of the next list, and not the tail of any other list, take A out, remove A in the lists in the merge
D + A + merge(B, B)
# the first element is the list is the head, not the tail, so B is taken
D + A + B

```

mro of **E** is computed as

```python3
E + merge(L(B), L(C), BC)
E + merge(B, C, BC)
E + B + merge(C, C)
E + B + C

```

mro of **F** is computed as

```python3
F + merge(L(D), L(E), DE)
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

```

## difference bwtween python2 and python3

in the following code

```python3
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

```

In Python 2, classes `A`, `B`, `C`, `D`, and `E` are not inherited from `object`. Even after Python 2.3, objects not inherited from `object` will not use the **C3** algorithm for **MRO**; the old-style Depth-First, Left-Most algorithm will be used

```python3
# ./python2.7
>>> f = F()
>>> f.who() # B is in front of E
I am B


```

While in Python 3, they both implicitly inherit from `object`. Objects inherited from `object` use the **C3** algorithm for **MRO**

```python3
# ./python3
>>> f = F()
>>> f.who() # E is in front of B
I am E

```

# creation of class

If we `dis` the above code

```python3
 26          96 LOAD_BUILD_CLASS
             98 LOAD_CONST              12 (<code object F at 0x10edf4150, file "mro.py", line 26>)
            100 LOAD_CONST              13 ('F')
            102 MAKE_FUNCTION            0
            104 LOAD_CONST              13 ('F')
            106 LOAD_NAME                6 (D)
            108 LOAD_NAME                7 (E)
            110 CALL_FUNCTION            4
            112 STORE_NAME               8 (F)

```

`LOAD_BUILD_CLASS` simply pushes the function `__build_class__` to stack

and the following opcodes push all the variables `__build_class__` needed to stack

`110 CALL_FUNCTION            4` calls the `__build_class__` to generate the class

The `__build_class__` will find the `metaclass`, `name`, `bases`, and `ns` (namespace) and delegate the call to the `metaclass.__call__` attribute

for example, the `class F`

```python3
metaclass: <class 'type'>
name: 'F'
bases: (<class '__main__.D'>, <class '__main__.E'>)
ns: {'__module__': '__main__', '__qualname__': 'F'}, cls: <class '__main__.F'>

```

In the example `class F`, what does `<class 'type'>.__call__` do?

The function is defined in `cpython/Objects/typeobject.c`.

The prototype is `static PyObject *type_call(PyTypeObject *type, PyObject *args, PyObject *kwds)`.

We can draw the following procedure according to the source code

![creation_of_class](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_class.png)

# creation of instance

If we add the following line to the end of the above code

```python3
f = F()

```

We can find another snippet of the `dis` result

```python3

 31         130 LOAD_NAME                9 (F)
            132 CALL_FUNCTION            0
            134 STORE_NAME              10 (f)

```

It simply calls the `__call__` attribute of `F`

```python3

>>> F.__call__
<method-wrapper '__call__' of type object at 0x7fa5fa725ec8>

```

![creation_of_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_instance.png)

# metaclass

In the above procedures, we can learn that the **metaclass** controls the creation of a **class**. The creation of an **instance** doesn't invoke the **metaclass**; it just calls the `__call__` attribute of the class to create the instance

![difference_between_class_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/difference_between_class_instance.png)

We can change the behavior of a class on the fly by manually defining a **metaclass**

```python3
class F(object):
    pass


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


print(Animal1.leg)  # 4
print(Normal.leg)  # 0
print(AnotherF)  # <class '__main__.F'>

```

pretty fun!

# read more
* [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)
* [The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)
