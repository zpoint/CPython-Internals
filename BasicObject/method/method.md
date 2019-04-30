# method

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)

### related file
* cpython/Objects/methodobject.c
* cpython/Include/methodobject.h


#### memory layout

There's a type named **builtin_function_or_method** in python, as the type name described, all the builtin function or method defined in the c level belong to **builtin_function_or_method**

	>>> print
    <built-in function print>
    >>> type(print)
    <class 'builtin_function_or_method'>

![layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/method/layout.png)

#### example

