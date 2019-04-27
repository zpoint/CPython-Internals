# Cpython Internals
* [English](https://github.com/zpoint/Cpython-Internals/blob/master/README.md)

这个仓库是笔者分析 [cpython](https://github.com/python/cpython) 源码的时候做的记录

笔者将尝试尽可能的多的用 图像/源代码注释 讲清楚 cpython 底层实现的细节

    # 基于 3.8.0a0 版本
    cd cpython
    git reset --hard ab54b9a130c88f708077c2ef6c4963b632c132b3


#### 收藏的博客 && 学习资料
* [rushter](https://rushter.com/)
* [YET ANOTHER PYTHON INTERNALS BLOG](https://pythoninternal.wordpress.com/)
* [CPython internals - Interpreter and source code overview(油管视频)](https://www.youtube.com/watch?v=LhadeL7_EIU&list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S)

#### 基本对象
- [x] [set](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_cn.md)
- [x] [list](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/list_cn.md)
- [x] [dict](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/dict/dict_cn.md)
- [x] [str/unicode](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/str_cn.md)
- [x] [tuple](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/tuple/tuple_cn.md)
- [x] [long/int](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/long/long_cn.md)
- [x] [bytes](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytes/bytes_cn.md)
- [ ] [bytearray](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytearray/bytearray_cn.md)
- [ ] float
- [ ] func
- [ ] method
- [ ] iter
- [ ] gen
- [ ] class
- [ ] complex
- [ ] enum
- [ ] file
- [ ] range
- [ ] slice

#### 模块

 - [ ] io
 	- [x] [fileio](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/fileio_cn.md)

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



