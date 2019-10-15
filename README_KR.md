# Cpython 내부

* [English](https://github.com/zpoint/CPython-Internals/blob/master/README.md)
* [简体中文](https://github.com/zpoint/CPython-Internals/blob/master/README_CN.md)
*  업데이트가 있을 때 알림을 받으려면, **Watch** 하세요.

이 저장소는 [cpython](https://github.com/python/cpython) 소스코드에 대한 나의 노트/블로그 입니다.

cpython 구현의 모든 상세한 부분들을 설명하도록 할 것 입니다.

    # 3.8.0a0 버전을 기반으로 합니다.
    cd cpython
    git reset --hard ab54b9a130c88f708077c2ef6c4963b632c132b3

다음의 내용들은 파이쎤 프로그래밍 경험이 있거나 파이썬 인터프리터 내부 구현에 관심이 있는 사람들을 위한 것이며, 초보자나 고급 자료가 필요하다면 [awesome-python-books](https://github.com/Junnplus/awesome-python-books) 을 참고하세요.

# 목차

* [객체들(Objects)](#객체들(Objects))
* [모듈들(Modules)](#모듈들(Modules))
* [라이브러리(Lib)](#라이브러리(Lib))
* [인터프리터(Interpreter)](#인터프리터(Interpreter))
* [확장(Extension)](#확장(Extension))
* [공부할 자료](#공부할-자료)


# 객체들(Objects)
 - [x] [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict.md)
 - [x] [long/int](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long.md)
 - [x] [unicode/str](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str.md)
 - [x] [set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set.md)
 - [x] [list(timsort)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list.md)
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
 - [x] [type(mro/metaclass/creation of class/instance)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type.md)

# 모듈들(Modules)

 - [ ] io
 	- [x] [fileio](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/fileio.md)
 - [ ] [pickle](https://github.com/zpoint/CPython-Internals/blob/master/Modules/pickle/pickle.md)

# 라이브러리(Lib)

 - [x] [re(regex)](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/re.md)
 - [ ] asyncio

# 인터프리터(Interpreter)

 - [x] [gil(Global Interpreter Lock)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md)
 - [x] [gc(Garbage Collection)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md)
 - [x] [memory management](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/memory_management.md)
 - [x] [descr(how does attribute access work/`__get__`/`__getattribute__`/`__getattr__`)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md)
 - [x] [exception(exception handling)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/exception.md)
 - [x] [module(how does import work)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/module.md)
 - [x] [frame](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame.md)
 - [x] [code](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code.md)
 - [x] [slots/`__slots__`(how does attribute initialized in the creation of class/instance)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/slot.md)
 - [x] [thread](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread.md)
 - [x] [PyObject(overview)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/pyobject.md)

# 확장(Extension)

 - [x] [C API(profile python code and write pure C extension)](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c.md)
 - [ ] Cython(C extension)
 - [x] [Boost C++ libaries (C\+\+ extension)](https://github.com/zpoint/Boost-Python-Examples)

# 문법(Grammar)

[< < Compilers > >](https://www.amazon.com/Compilers-Principles-Techniques-Tools-2nd/dp/0321486811) 와 [< < SICP > >](https://www.amazon.com/Structure-Interpretation-Computer-Programs-Engineering/dp/0262510871) 를 읽어 본 후 이런 종류의 것들에 대한 이해가 높아지면, 이 부분을 업데이트 할 것 입니다.

그 동안, 내 일상적인 작업의 우선 순위가 더 높으므로, 이 부분의 업데이트를 보려면 몇 달이 필요할 수 있습니다.

# 공부할 자료

읽어본 자료들만 추천할 것 입니다.

* [CPython internals - Interpreter and source code overview(youtube video)](https://www.youtube.com/watch?v=LhadeL7_EIU&list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S)
* [< < Inside The Python Virtual Machine > >](https://leanpub.com/insidethepythonvirtualmachine)
* [< < Python源码剖析 > >](https://book.douban.com/subject/3117898/)
* [rushter(blog/eng)](https://rushter.com/)
* [YET ANOTHER PYTHON INTERNALS BLOG(blog/eng)](https://pythoninternal.wordpress.com/)
* [Junnplus(blog/cn)](https://github.com/Junnplus/blog/issues)
* [manjusaka(blog/cn)](https://manjusaka.itscoder.com/)
