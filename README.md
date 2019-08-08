# Cpython Internals

* [简体中文](https://github.com/zpoint/CPython-Internals/blob/master/README_CN.md)
*  **Watch** this repo if you need to be notified when there's update

This repository is my notes/blog for [cpython](https://github.com/python/cpython) source code

Trying to illustrate every detail of cpython implementation

    # based on version 3.8.0a0
    cd cpython
    git reset --hard ab54b9a130c88f708077c2ef6c4963b632c132b3

The following contents are suitable for those who have python programming experience and interested in internal of python interpreter, for those who needs beginner or advanced material please refer to [awesome-python-books](https://github.com/Junnplus/awesome-python-books)

# Table of Contents

* [Objects](#Objects)
* [Modules](#Modules)
* [Lib](#Lib)
* [Interpreter](#Interpreter)
* [Extension](#Extension)
* [Learning material](#Learning-material)


# Objects
 - [x] [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict.md)
 - [x] [long/int](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long.md)
 - [x] [unicode/str](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str.md)
 - [x] [set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set.md)
 - [ ] [list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list.md)
 - [x] [tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md)
 - [x] [bytes](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/bytes.md)
 - [x] [bytearray(buffer protocol)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/bytearray.md)
 - [x] [float](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float.md)
 - [x] [func(user-defined method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func.md)
 - [x] [method(builtin method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method.md)
 - [x] [iter](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter.md)
 - [x] [gen(generator/coroutine/async generator)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen.md)
 - [x] [class(bound method/classmethod/staticmethod)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class.md)
 - [x] [complex](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/complex.md)
 - [x] [enum](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/enum.md)
 - [x] [type(mro/metaclass)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type.md)

# Modules

 - [ ] io
 	- [x] [fileio](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/fileio.md)
 - [ ] [pickle](https://github.com/zpoint/CPython-Internals/blob/master/Modules/pickle/pickle.md)

# Lib

 - [x] [re(regex)](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/re.md)
 - [ ] asyncio

# Interpreter

 - [x] [gil(Global Interpreter Lock)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md)
 - [x] [gc(Garbage Collection)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)
 - [x] [memory management](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/memory_management.md)
 - [x] [descr(how does attribute access work/`__get__`/`__getattribute__`/`__getattr__`)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md)
 - [x] [exception(exception handling)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/exception.md)
 - [x] [module(how does import work)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/module.md)
 - [x] [frame](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)
 - [x] [code](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code.md)
 - [x] [slot/`__slots__`](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/slot.md)
 - [x] [thread](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread.md)
 - [x] [PyObject(overview)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/pyobject.md)

# Extension

 - [x] [C API(profile python code and write pure C extension)](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c.md)
 - [ ] Cython(C extension)
 - [x] [Boost C++ libaries (C\+\+ extension)](https://github.com/zpoint/Boost-Python-Examples)

# Grammar

I will come back to this part when I finish reading [< < Compilers > >](https://www.amazon.com/Compilers-Principles-Techniques-Tools-2nd/dp/0321486811) and [< < SICP > >](https://www.amazon.com/Structure-Interpretation-Computer-Programs-Engineering/dp/0262510871) and have a better understanding of this kind of stuffs

In the meantime, my routine work will have a higher priority, so you may need months to see updates in this part


# Learning material

I will only recommend what I've read

* [rushter](https://rushter.com/)
* [YET ANOTHER PYTHON INTERNALS BLOG](https://pythoninternal.wordpress.com/)
* [CPython internals - Interpreter and source code overview](https://www.youtube.com/watch?v=LhadeL7_EIU&list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S)
* [< < Inside The Python Virtual Machine > >](https://leanpub.com/insidethepythonvirtualmachine)
* [< < Python源码剖析 > >](https://book.douban.com/subject/3117898/)