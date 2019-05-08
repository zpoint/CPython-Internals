# Cpython Internals

* [English](https://github.com/zpoint/CPython-Internals/blob/master/README.md)

这个仓库是笔者分析 [cpython](https://github.com/python/cpython) 源码的时候做的记录/博客

笔者将尝试尽可能的讲清楚 cpython 底层实现的细节

    # 基于 3.8.0a0 版本
    cd cpython
    git reset --hard ab54b9a130c88f708077c2ef6c4963b632c132b3

### 目录

* [基本对象](#基本对象)
* [模块](#模块)
* [库](#库)
* [解释器相关](#解释器相关)
* [学习资料](#学习资料)


#### 基本对象
- [x] [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_cn.md)
- [x] [long/int](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long_cn.md)
- [x] [unicode/str](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str_cn.md)
- [x] [set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_cn.md)
- [x] [list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_cn.md)
- [x] [tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md)
- [x] [bytes](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/bytes_cn.md)
- [x] [bytearray](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/bytearray_cn.md)
- [x] [float](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float_cn.md)
- [x] [func(user-defined method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func_cn.md)
- [x] [method(builtin method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method_cn.md)
- [x] [iter](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter_cn.md)
- [x] [gen](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen_cn.md)
- [x] [class(bound method)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class_cn.md)
- [x] [complex](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/complex_cn.md)
- [x] [enum](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/enum_cn.md)

#### 模块

 - [ ] io
 	- [x] [fileio](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/fileio_cn.md)

#### 库

 - [ ] re
 - [ ] asyncio

#### 解释器相关

 - [ ] frame
 - [ ] code
 - [ ] descr
 - [ ] exception
 - [ ] module
 - [ ] namespace
 - [ ] GIL

#### 学习资料

以下的资料笔者遵循先阅读后推荐的原则

若页面无法打开请自行搭梯子O(∩_∩)O

* [rushter](https://rushter.com/)
* [YET ANOTHER PYTHON INTERNALS BLOG](https://pythoninternal.wordpress.com/)
* [CPython internals - Interpreter and source code overview(油管视频)](https://www.youtube.com/watch?v=LhadeL7_EIU&list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S)
* [< < Inside The Python Virtual Machine > >](https://leanpub.com/insidethepythonvirtualmachine)
* [< < Python源码剖析 > >](https://book.douban.com/subject/3117898/)

