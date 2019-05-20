# exception

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
	* [base_exception](#base_exception)
* [exception handling](#exception-handling)

#### related file

* cpython/Include/cpython/pyerrors.h
* cpython/Objects/exceptions.c

#### memory layout

##### base_exception

there are various exception types defined in `Include/cpython/pyerrors.h`, the most widely used **PyBaseExceptionObject**(also the base part of any other exception type)

![base_exception](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/base_exception.png)


