# PyObject

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [overview](#overview)
* [ceval](#ceval)
* [interpreter and thread](#interpreter-and-thread)
* [read more](#read-more)

# related file

* cpython/Include/object.h
* cpython/Include/cpython/object.h
* cpython/Python/ceval.c
* cpython/Include/ceval.h
* cpython/Python/pythonrun.c

# memory layout

This is the layout of **PyObject**. It's the basic part of every other Python object

Every object in Python can be cast to **PyObject**, e.g., list, tuple, etc.

`PyObject_HEAD_EXTRA` is the head of the double linked list, it's used for keeping track in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_refcnt` stores reference count of the current object, it's also used in [Garbage Collection](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)

`ob_type` is a pointer to its type object. For example, `type("abc") ===> str`, `"abc"` is the PyObject, and `ob_type` in the PyObject will point to `str`

![PyObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyObject.png)

# overview

Whenever you execute a Python program from a `.py` file, the compile phase will translate the human-readable source code to a form called Python bytecode

The `pyc` file will be generated if necessary, and the interpreter phase will start the main loop, executing the bytecode in the `pyc` file line by line

The compiled `pyc` file will not boost the runtime of the program. Instead, the load time of the program will be faster because it doesn't need to generate the `pyc` file again

According to [PEP 3147](https://www.python.org/dev/peps/pep-3147/), after Python 3.3, `pyc` files will only be generated through the `import` mechanism, and `pyc` files will exist under the `__pycache__` directory

![executePy](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/executePy.png)

# ceval

The main loop of the interpreter is defined in `cpython/Python/ceval.c`

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

We can draw the procedure

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/ceval.png)

# interpreter and thread

This is the definition of **PyInterpreterState**

![PyInterpreterState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyInterpreterState.png)

This is the definition of **PyThreadState**

![PyThreadState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyThreadState.png)

If we have two threads currently running

![organize](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/organize.png)

# read more
* [pep-3147](https://www.python.org/dev/peps/pep-3147/)
* [Junnplus's blog: Python中的code对象](https://github.com/Junnplus/blog/issues/16)