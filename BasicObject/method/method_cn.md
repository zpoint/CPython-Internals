# method

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
	* [print](#print)
* [PyMethodDef 中的字段](#PyMethodDef-中的字段)
* [free_list(缓冲池)](#free_list)
* [classmethod](#classmethod)
* [staticmethod](#staticmethod)

# 相关位置文件
* cpython/Objects/methodobject.c
* cpython/Include/methodobject.h
* cpython/Python/bltinmodule.c
* cpython/Objects/call.c

# 内存构造

python 中有一个类型叫做 **builtin_function_or_method**, 正如如类型名称所描述的一般, 所有在c语言层级定义的的内建函数或者方法都属于类型 **builtin_function_or_method**

```python3
>>> print
<built-in function print>
>>> type(print)
<class 'builtin_function_or_method'>

```

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/layout.png)

# 示例

## print

我们来看一小段代码段先

```c
#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

```

**PyCFunction** 在 c 语言中是一个类型, 这个类型可以表示任何接受两个 PyObject * 作为参数, 并返回一个 PyObject * 作为返回对象的函数

```c
// 一个 c 函数, 函数名为 builtin_print
static PyObject *
builtin_print(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

// "print" 这个名称被定义在了一个叫做 builtin_methods 的 c 数组中
static PyMethodDef builtin_methods[] = {
...
{"print",           (PyCFunction)(void(*)(void))builtin_print,      METH_FASTCALL | METH_KEYWORDS, print_doc},
...
}

```

![print](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print.png)

一个 **PyMethodDef** 会与一个 **module** 对象, 和一个 **self** 对象相关联, 关联之后就可以用一个 **PyCFunctionObject** 对象来表示

用户实际上在交互界面可以看到的对象在 c 层级被定义为类型 **PyCFunctionObject**

![print2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print2.png)

我们来看看每个字段的意义

**m_self** 字段中的类型是 **module**, 而 **m_module** 字段中的类型是 **str**

![print3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/print3.png)

# PyMethodDef 中的字段

## ml_name

如上图所示, **ml_name** 字段表示的是这个内建函数的名称, 他就是一个 null 结尾的 c 语言标准字符串

## ml_meth

指向真正干活的函数指针

## ml_flags

用二进制位来标记这个函数需要被如何调用

**call.c** 文件里面以 **_PyMethodDef_** 开头的函数会根据这些二进制位, 得出真正需要被传入的参数以及调用方式等信息, 根据这些信息去取出 python 层传入的参数, 再去调用 c 层的 **PyCFunction**

需要更多详情表示每个位的作用的同学可以参考 [c-api Common Object Structures](https://docs.python.org/3/c-api/structures.html)

| 标记名称 | 标记值 | 意义 |
| - | :-: | -: |
| METH_VARARGS | 0x0001| c 函数类型为 PyCFunction, 接受两个 PyObject* 作为参数 |
| METH_KEYWORDS | 0x0002 | c 函数类型为 PyCFunctionWithKeywords, 接受三个参数, self, args 和一个字典对象表示 keyword arguments |
| METH_NOARGS | 0x0004 | 表示不接受任何参数, c 函数类型为 PyCFunction(外层调用 c 函数时会传入 self 和 NULL 值) |
| METH_O | 0x0008 | 接受单个参数, c 函数类型为 PyCFunction |
| METH_CLASS | 0x0010 | 取出第一个对象的类型传入, 而不是第一个对象的实例去传入, 就是我们常用的@classmethod 干的活 |
| METH_STATIC | 0x0020 | 第一个对象会传入空的值, 我们常用的 @staticmethod 干的活 |
| METH_COEXIST | 0x0040 | 重复定义时替代原本存在的, 而不是跳过 |

# free_list

```c
static PyCFunctionObject *free_list = NULL;
static int numfree = 0;
#ifndef PyCFunction_MAXFREELIST
#define PyCFunction_MAXFREELIST 256
#endif

```

cpython 使用了一个长度为 256 的缓冲池来存储释放掉的对象以供再度循环使用, free_list 是一个单链表, 利用 PyCFunctionObject 上面的 **m_self** 字段一直往下串在一起

相似的技术在 [float-free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float_cn.md#free_list) 对象上也使用过, float 使用的是 **ob_type** 字段串联起来, 这里就不做图说明了, 有兴趣的同学可以直接点解链接看里面的图片

# classmethod

# staticmethod

其实他们在分析 [func](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/dunc_cn.md) 对象时就已经出现了, 但是他们和 **classobject** 文件中的关联度较高, 我会在后续看 [class](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/class_cn.md) 时分析这两个家伙