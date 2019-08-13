# Cpython Internals

* [English](https://github.com/zpoint/CPython-Internals/blob/master/README.md)
* 如果你需要接收更新通知, 点击右上角的 **Watch**, 当有文章更新时会在 issue 发布相关标题和链接
* 如果有任何链接无法打开请自行搭梯子O(∩_∩)O

这个仓库是笔者分析 [cpython](https://github.com/python/cpython) 源码的时候做的记录/博客

笔者将尝试尽可能的讲清楚 cpython 底层实现的细节

    # 基于 3.8.0a0 版本
    cd cpython
    git reset --hard ab54b9a130c88f708077c2ef6c4963b632c132b3

这里的内容适用于有过 python 编程经验并且对解释器的实现感兴趣的同学, 如果你需要的的是入门到进阶之类的学习资料, 请参考 [awesome-python-books](https://github.com/Junnplus/awesome-python-books/blob/master/README-ZH_CN.md)


# 目录

* [基本对象](#基本对象)
* [模块](#模块)
* [库](#库)
* [解释器相关](#解释器相关)
* [扩展](#扩展)
* [学习资料](#学习资料)


# 基本对象
- [x] [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_cn.md)
- [x] [long/int](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long_cn.md)
- [x] [unicode/str](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str_cn.md)
- [x] [set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_cn.md)
- [x] [list(timsort)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_cn.md)
- [x] [tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md)
- [x] [bytes](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/bytes_cn.md)
- [x] [bytearray(buffer protocol)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/bytearray_cn.md)
- [x] [float](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float_cn.md)
- [x] [func(user-defined method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func_cn.md)
- [x] [method(builtin method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method_cn.md)
- [x] [iter](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter_cn.md)
- [x] [gen(generator/coroutine/async generator)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen_cn.md)
- [x] [class(bound method/classmethod/staticmethod)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class_cn.md)
- [x] [complex](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/complex_cn.md)
- [x] [enum](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/enum_cn.md)
- [x] [type(mro/metaclass)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type_cn.md)

# 模块

 - [ ] io
 	- [x] [fileio](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/fileio_cn.md)
 - [ ] [pickle](https://github.com/zpoint/CPython-Internals/blob/master/Modules/pickle/pickle_cn.md)

# 库

 - [x] [re(正则)](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/re_cn.md)
 - [ ] asyncio

# 解释器相关

 - [x] [gil(全局解释器锁)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_cn.md)
 - [x] [gc(垃圾回收机制)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc_cn.md)
 - [x] [memory management(内存管理机制)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/memory_management_cn.md)
 - [x] [descr(访问(类/实例)属性时发生了什么/`__get__`/`__getattribute__`/`__getattr__`)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md)
 - [x] [exception(异常处理机制)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/exception/exception_cn.md)
 - [x] [module(import实现机制)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/module_cn.md)
 - [x] [frame](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame_cn.md)
 - [x] [code](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code_cn.md)
 - [x] [slots/`__slots__`](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/slot_cn.md)
 - [x] [thread(线程)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread_cn.md)
 - [x] [PyObject(基础篇/概述)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/pyobject_cn.md)

# 扩展

 - [x] [C API(python 性能分析和 C 扩展)](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c_cn.md)
 - [ ] Cython(C extension)
 - [x] [Boost C++ libaries (C\+\+ extension)](https://github.com/zpoint/Boost-Python-Examples)

# 语法

这一部分我计划在读完 [< < Compilers > >](https://www.amazon.com/Compilers-Principles-Techniques-Tools-2nd/dp/0321486811) 和 [< < SICP > >](https://www.amazon.com/Structure-Interpretation-Computer-Programs-Engineering/dp/0262510871) 之后对这类知识有更好的理解之后再来更新, 与此同时我还需要处理优先级更高的日常工作, 所以可能数月之后这部分才会有更新


# 学习资料

以下的资料笔者遵循先阅读后推荐的原则

* [CPython internals - Interpreter and source code overview(油管视频)](https://www.youtube.com/watch?v=LhadeL7_EIU&list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S)
* [< < Inside The Python Virtual Machine > >](https://leanpub.com/insidethepythonvirtualmachine)
* [< < Python源码剖析 > >](https://book.douban.com/subject/3117898/)
* [rushter(blog/eng)](https://rushter.com/)
* [YET ANOTHER PYTHON INTERNALS BLOG(blog/eng)](https://pythoninternal.wordpress.com/)
* [Junnplus(blog/中文)](https://github.com/Junnplus/blog/issues)
* [manjusaka(blog/中文)](https://manjusaka.itscoder.com/)
