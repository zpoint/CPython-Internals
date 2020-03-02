# PyObject

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [overview](#overview)
* [ceval](#ceval)
* [intepreter and thread](#intepreter-and-thread)
* [read more](#read-more)

# related file

* cpython/Include/object.h
* cpython/Include/cpython/object.h
* cpython/Python/ceval.c
* cpython/Include/ceval.h
* cpython/Python/pythonrun.c

# memory layout

this is the layout of **PyObject**, it's the basic part of every other python object

every object in python can be cast to **PyObject**, i.e, list, tuple and etc

`PyObject_HEAD_EXTRA` is the head of the double linked list, it's used for keeping track in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_refcnt` stores reference count of the current object, it's also used in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_type` is a pointer to it's type object, i.e, `type("abc") ===> str`, `"abc"` is the PyObject, and `ob_type` in the PyObject will points to `str`

![PyObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyObject.png)

# overview

whenever you execute a python program for a `.py` file, the compile phase will translate the human readable source code to a form called python bytecode

the `pyc` file will be generated if necessary, and the interpreter phase will start the main loop, execute the bytecode in the `pyc` file line by line

the compiled `pyc` file will not boost the run time of the program, instead, the load time of the program will be faster because it doesn't need to generate `pyc` file again

according to [pep-3147](https://www.python.org/dev/peps/pep-3147/), after python 3.3, `pyc` file will only be generated in the `import` mechanism, and `pyc` file will exists under the `__pycache__` directory

![executePy](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/executePy.png)

# ceval

the main loop of the interpreter is defined in `cpython/Python/ceval.c`

```c
main_loop:
    for (;;) {
    	// jump to fast_next_opcode if necessary
    	// check for signal hndler/async io handler
        // drop gil if needed
		fast_next_opcode:
            switch (opcode) {
                /* ... */
            }
    }

```

we can draw the procedure

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/ceval.png)

# intepreter and thread

this is the defination of **PyInterpreterState**

![PyInterpreterState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyInterpreterState.png)

this is the defination of **PyThreadState**

![PyThreadState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyThreadState.png)

if we have two thread currently running

![organize](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/organize.png)

# read more
* [pep-3147](https://www.python.org/dev/peps/pep-3147/)
* [Junnplus's blog: Python中的code对象](https://github.com/Junnplus/blog/issues/16)