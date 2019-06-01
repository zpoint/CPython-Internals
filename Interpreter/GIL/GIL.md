# gil

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)

#### related file

* cpython/Python/ceval.c
* cpython/Python/ceval_gil.h
* cpython/Include/internal/pycore_gil.h

#### memory layout

this is the defination of the **Global Interpreter Lock**

> In CPython, the global interpreter lock, or GIL, is a mutex that protects access to Python objects, preventing multiple threads from executing Python bytecodes at once. This lock is necessary mainly because CPython's memory management is not thread-safe. (However, since the GIL exists, other features have grown to depend on the guarantees that it enforces.)

Due to some performance issue in multithreaded programs, the implementation of the **GIL** has changed a lot after python3.2

thread scheduling before python3.2 (screenshot from [Understanding the Python GIL(youtube video)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

![old_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/old_gil.png)

thread scheduling after python3.2

![new_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)

for those who are interested in detail, please refer to [Understanding the Python GIL(article)](http://www.dabeaz.com/GIL/)

![git_layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_layout.png)

