# PyObject

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [overview](#overview)
* [ceval](#ceval)
* [intepreter and thread](#intepreter-and-thread)

# related file

* cpython/Include/object.h
* cpython/Include/cpython/object.h
* cpython/Python/ceval.c
* cpython/Python/pythonrun.c

# memory layout

this is the layout of **PyObject**, it's the basic part of every other python type

every object in python can be cast to **PyObject**, i.e, list, tuple and etc

`PyObject_HEAD_EXTRA` is the head of the double linked list, it's used for keeping track in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_refcnt` stores reference count of the current object, it's also used in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_type` is a pointer to it's type object, i.e, `type("abc") ===> str`, `"abc"` is the PyObject, and `ob_type` in the PyObject will points to `str`

![PyObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyObject.png)

# overview

whenever you execute a python program for a `.py` file, the compile phase will translate the human readable source code to a form called python bytecode

the `pyx` file will be generated if necessary, and the interpreter phase will start the main loop, execute the bytecode in the `pyx` file line by line

the compiled `pyx` file will not boost the run time of the program, instead, the load time of the program will be faster because it doesn't need to generate `pyx` file again

![executePy](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/executePy.png)

# ceval

