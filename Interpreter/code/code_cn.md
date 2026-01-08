# code

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [字段](#字段)
	* [示例](#示例)
	* [co_code](#co_code)
	* [co_lnotab 和 co_firstlineno](#co_lnotab-和-co_firstlineno)
	* [co_zombieframe](#co_zombieframe)
	* [co_extra](#co_extra)

# 相关位置文件
* cpython/Objects/codeobject.c
* cpython/Include/codeobject.h

# 内存构造

> 这篇文章这个格式的文字表示引用, 凡是以这个格式出现的, 都翻译自下面 quora 的回答

需要更多的信息请参考 [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python)

> CPython 内部使用 code 对象来表示一段可执行的 python 代码段, 比如函数, 模块, 一个类, 或者迭代器. 当你执行一段代码的时候, 这段代码会被解析, 编译, 之后变成一个 code 对象, 这个 code 对象可以被 python 虚拟机执行

![layout](https://img-blog.csdnimg.cn/20190208112516130.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# 字段

## 示例

我们来跑一个用 python 写的示例看看

```python3
def a(x, y, *args,  z=3, **kwargs):
    def b():
        bbb = 4
        print(x, k, bbb)
    k = 4
    print(x, y, args, kwargs)

print("co_argcount", a.__code__.co_argcount)
print("co_kwonlyargcount", a.__code__.co_kwonlyargcount)
print("co_nlocals", a.__code__.co_nlocals)
print("co_stacksize", a.__code__.co_stacksize)
print("co_flags", a.__code__.co_flags)
print("co_firstlineno", a.__code__.co_firstlineno)
print("co_code", a.__code__.co_code)
print("co_consts", a.__code__.co_consts)
print("co_names", a.__code__.co_names)
print("co_varnames", a.__code__.co_varnames)
print("co_freevars", a.__code__.co_freevars)
print("co_cellvars", a.__code__.co_cellvars)

print("co_cell2arg", a.__code__.co_filename)
print("co_name", a.__code__.co_name)
print("co_lnotab", a.__code__.co_lnotab)

print("\n\n", a.__code__.co_consts[1])
print("co_freevars", a.__code__.co_consts[1].co_freevars)
print("co_cellvars", a.__code__.co_consts[1].co_cellvars)

```

输出

```python3
co_argcount 2
co_kwonlyargcount 1
co_nlocals 6
co_stacksize 5
co_flags 15
co_firstlineno 1
co_code b'\x87\x00\x87\x01f\x02d\x01d\x02\x84\x08}\x05d\x03\x89\x00t\x00\x88\x01|\x01|\x03|\x04\x83\x04\x01\x00d\x00S\x00'
co_consts (None, <code object b at 0x10228e810, file "/Users/zpoint/Desktop/cpython/bad.py", line 2>, 'a.<locals>.b', 4)
co_names ('print',)
co_varnames ('x', 'y', 'z', 'args', 'kwargs', 'b')
co_freevars ()
co_cellvars ('k', 'x')
co_cell2arg /Users/zpoint/Desktop/cpython/bad.py
co_name a
co_lnotab b'\x00\x01\x0e\x03\x04\x01'


 <code object b at 0x10228e810, file "/Users/zpoint/Desktop/cpython/bad.py", line 2>
co_freevars ('k', 'x')
co_cellvars ()


```

从上面的输出和 [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) 中的回答, 我们可以了解到部分字段的意义

**co_argcount**

> 当前函数接受的参数个数, 不包括 `*args` 和 `**kwargs`. python 虚拟机中函数的运行机制是把所有的参数压如栈中, 之后执行 CALL_FUNCTION 这个字节码. co_argcount 在运行的时候可以用来校验函数传递的参数数量等信息是否正确

**co_kwonlyargcount**

> [keyword-only arguments](https://www.python.org/dev/peps/pep-3102/) 的数量

**co_nlocals**

这个字段和 [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame_cn.md) 的 **f_localsplus** 非常相关

> 局部变量的数量. 据我(quora 回答的作者)所知他就是 co_varnames 这个对象的长度, co_nlocals 的作用是当函数被调用时, 可以预先知道需要为局部变量分配多少栈空间

**co_stacksize**

这个字段和 [frame object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame_cn.md) 的 **f_valuestack** 非常相关

> 一个整数表示函数运行时最多会使用多少栈空间, 这个字段是非常必要的, 因为虚拟机中函数运行的栈空间是在 code 对象运行之前事先分配的, 如果 co_stacksize 过小, 这个函数执行过程中可能造成栈溢出, 会产生意想不到的结果

**co_flags**

> 一个整数, 表示函数本身一些属性的组合

**co_consts**

> 一个元组对象, 包含了函数中用到的所有常量. 比如 整数, 字符串对象, 和布尔对象. 这个元组里面的内容会在字节码 LOAD_CONST 执行的时候用到, LOAD_CONST 后面会跟一个参数, 这个参数表示 co_consts 的下标位置

**co_names**

> 一个元组, 元组里的对象都是字符串对象, 存储着属性, 全局变量名称, 导入的名称等信息. 用到这部分信息的字节码(比如 LOAD_ATTR)后面跟一个参数, 表示这个 co_names 的下标位置

**co_varnames**

> 一个元组对象, 包括了函数用到的参数名称和局部变量名称, 参数名称包括普通参数, *args 和 *\**kwargs, 这些名称在元组里的顺序和在函数对象中第一次使用到的顺序相同

**co_cellvars** and **co_freevars**

> 这两个字段在包含有 enclosing scope 的时候使用, co_freevars 如果当前函数是 enclosing scope 内部的函数, 当前使用到的定义于外层的变量名称
> co_cellvars 如果当前函数含有 enclosing scope, 表示 enclosing scope 里面的函数使用到的定义于当前函数的的变量名称

## co_code

当你执行这个命令时  `./python.exe -m dis code.py`

在 `Lib/dis.py` 中的函数 **_unpack_opargs** 会做如下转换

你打开这个文件 `Include/opcode.h`, 能看 `#define HAVE_ARGUMENT            90` 和 `#define HAS_ARG(op) ((op) >= HAVE_ARGUMENT)` 这两个定义, 表示值大于 **90** 的字节码是含有参数的字节码, 而值小于 **90** 的字节码是不含参数的字节码

```python3
def _unpack_opargs(code):
    # code 示例: b'd\x01}\x00t\x00\x88\x01\x88\x00|\x00\x83\x03\x01\x00d\x00S\x00'
    extended_arg = 0
    for i in range(0, len(code), 2):
        op = code[i]
        if op >= HAVE_ARGUMENT:
            arg = code[i+1] | extended_arg
            extended_arg = (arg << 8) if op == EXTENDED_ARG else 0
        else:
            arg = None
        # yield 示例: 0 100 1
        yield (i, op, arg)

```

所以 **co_code** 是二进制方式存储的字节码和对应的参数

```python3
>>> c = b'd\x01}\x00t\x00\x88\x01\x88\x00|\x00\x83\x03\x01\x00d\x00S\x00'
>>> c = list(bytearray(c))
>>> c
[100, 1, 125, 0, 116, 0, 136, 1, 136, 0, 124, 0, 131, 3, 1, 0, 100, 0, 83, 0]

```

上面的二进制值根据前面的转换函数可以转换为

```python3
0 100 1  (LOAD_CONST)
2 125 0  (STORE_FAST)
4 116 0  (LOAD_GLOBAL)
6 136 1  (LOAD_DEREF)
8 136 0  (LOAD_DEREF)
10 124 0 (LOAD_FAST)
12 131 3 (CALL_FUNCTION)
14 1 None(POP_TOP)
16 100 0 (LOAD_CONST)
18 83 None(RETURN_VALUE)

```

## co_lnotab 和 co_firstlineno

**co_firstlineno**

> 1 开始数的下标, 表示这个 code 对象生成的时候对应的文件行数, 和 co_lnotab 结合起来可以用来计算当前字节码所处的文件的行数(异常处理, 错误追踪的时候使用到)

**co_lnotab**

> 行数信息表, 存储和压缩后的字节码和行数对应的信息

我们来看个示例

**co_lnotab** 中第一对值 (0, 1) 表示字节码位置 0, 行数 1 + co_firstlineno(7) == 8

**co_lnotab** 中第二对值 (4, 1) 表示字节码位置 4, 行数 1 + 8(前一个行数位移) == 9

```python3
import dis

def f1(x):
    x = 3
    y = 4

def f2(x):
    x = 3
    y = 4

print(f2.__code__.co_firstlineno) # 7
print(repr(list(bytearray(f2.__code__.co_lnotab)))) # [0, 1, 4, 1]
print(dis.dis(f2))
"""
0 LOAD_CONST               1 (3)
2 STORE_FAST               0 (x)
4 LOAD_CONST               2 (4)
6 STORE_FAST               1 (y)
8 LOAD_CONST               0 (None)
"""

```

## co_zombieframe

你可以在 `Objects/frameobject.c` 找到以下注释

需要更多详细的信息, 请参考 [frame 对象(zombie frame)](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/frame_cn.md#zombie-frame)

> 每个 code 对象都会保持一个 "zombie" frame, 这样做能保持一个 frame 对象分配的空间和初始化的信息不变
> 下一次同一个 code 对象需要创建一个 frame 对象的时候, 直接用 "zombie" frame 上的对象, 可以节省 malloc/ realloc 的开销 和 初始化字段的开销

## co_extra

这个字段存储了一个指向 **_PyCodeObjectExtra** 对象的指针

```c
typedef struct {
    Py_ssize_t ce_size;
    void *ce_extras[1];
} _PyCodeObjectExtra;

```

这个对象有一个表示大小的字段, 和一个指向 (void *) 的指针数组, 理论上他可以存下任何对象

通常, 它被用来作为一个函数指针, 指向和解释器绑定的函数, 在 debug 或者 JIT 场景下使用

需要更多信息请参考 [PEP 523 -- Adding a frame evaluation API to CPython](https://www.python.org/dev/peps/pep-0523/)