# PyObject

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [概述](#概述)
* [ceval](#ceval)
* [intepreter 和 thread](#intepreter-和-thread)
* [更多资料](#更多资料)

# 相关位置文件

* cpython/Include/object.h
* cpython/Include/cpython/object.h
* cpython/Python/ceval.c
* cpython/Include/ceval.h
* cpython/Python/pythonrun.c

# 内存构造

这是 **PyObject** 的内存构造, 它是所有 python 对象都会拥有的基础部分

每一个 python 对象都可以被强制转换为 **PyObject** 指针, 比如 list, tuple 对象等

`PyObject_HEAD_EXTRA` 是双端链表的表头, 它的作用是在 [垃圾回收](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc_cn.md) 机制中追踪到所有 python 对象

`ob_refcnt` 存储了当前的 python 对象的引用计数, 它也在 [垃圾回收](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc_cn.md) 的第一部分发挥了至关重要的作用

`ob_type` 是一个指向对象所属类型的指针, i.e, `type("abc") ===> str`, `"abc"` 是这个 PyObject, 在这个 PyObject 中的 `ob_type` 则会指向 `str`

![PyObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyObject.png)

# 概述

python 解释器不仅仅是一个解释器, 它包括很多个部分

当你通过 `.py` 文件来执行一个 python 程序时, 编译的部分会把你写的源代码转换为一种叫做 python 字节码的形式, import 目录下的文件中的字节码会被缓存在 `pyc` 文件中, 解释器会开始 main loop 循环, 一个字节码一个字节码的读取, 并执行

这个 `pyc` 文件并不会提高你的程序运行速度, 它只会提高你程序的加载速度, 你只要没有修改过源代码文件, 则下次启动可以跳过生成 `pyc` 的部分, 仅此而已

根据 [pep-3147](https://www.python.org/dev/peps/pep-3147/) 所描述的, 在 python 版本 3.3 之后, `pyc` 文件只会在 `import` 机制下产生, 并且 `pyc` 文件会保存在 `__pycache__` 目录下

![executePy](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/executePy.png)

# ceval

main loop 循环主要定义在如下位置 `cpython/Python/ceval.c`

```c
main_loop:
    for (;;) {
    	// 必要情况下跳转到 fast_next_opcode
    	// 检查是否有 signal hndler/async io handler 需要处理
        // 检查是否需要释放 gil
		fast_next_opcode:
            switch (opcode) {
                /* ... */
            }
    }

```

我们可以画出流程

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/ceval.png)

# intepreter 和 thread

这是 **PyInterpreterState** 的定义

![PyInterpreterState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyInterpreterState.png)

这是 **PyThreadState** 的定义

![PyThreadState](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/PyThreadState.png)

如果我们当前有两个线程在运行中

![organize](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/organize.png)

# 更多资料
* [pep-3147](https://www.python.org/dev/peps/pep-3147/)
* [Junnplus's blog: Python中的code对象](https://github.com/Junnplus/blog/issues/16)