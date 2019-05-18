# frame

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
	* [f_valuestack/f_stacktop/f_localsplus](#f_valuestack/f_stacktop/f_localsplus)
	* [f_blockstack](#f_blockstack)
	* [f_back](#f_back)
* [free_list 机制](#free_list-机制)
	* [zombie frame](#zombie-frame)
	* [free_list(缓冲池)](#free_list-sub)

#### 相关位置文件
* cpython/Objects/frameobject.c
* cpython/Include/frameobject.h

#### 内存构造

**PyFrameObject** 是 python 虚拟机使用的栈帧对象, 它包含了当前所执行的代码所需要的空间, 参数, 不同作用域的变量, try 的信息等

需要更多有关栈帧的信息请参考 [stack frame strategy](http://en.citizendium.org/wiki/Memory_management)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/layout.png)

#### 示例

每当你在解释器中做一次函数调用时, 会相应的创建一个新的 **PyFrameObject** 对象, 这个对象就是当前函数调用的栈帧对象

在一个函数调用中追踪栈帧的变化不是很直观, 我会用一个 [generator 对象](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen_cn.md) 来追踪栈帧的变化

你可以通过以下命令获得当前执行环境中的栈帧对象 `sys._current_frames()`

如果你想知道 **PyFrameObject** 中每个字段的意义, 请参考 [Junnplus' blog](https://github.com/Junnplus/blog/issues/22) 或者直接阅读源代码

##### f_valuestack/f_stacktop/f_localsplus

**PyFrameObject** 对象的大小是不固定的, 你可以把它强制转换为类型 **PyVarObject**, **ob_size** 存储这个对象动态分配部分的大小, 这个大小是所关联的 **code** 对象决定的

    Py_ssize_t extras, ncells, nfrees;
    ncells = PyTuple_GET_SIZE(code->co_cellvars);
    nfrees = PyTuple_GET_SIZE(code->co_freevars);
    extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
    /* 忽略 */
    if (free_list == NULL) { /* omit */
        f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, extras);
    }
    else { /* 忽略 */
    	PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);
    }
    extras = code->co_nlocals + ncells + nfrees;
    f->f_valuestack = f->f_localsplus + extras;
    for (i=0; i<extras; i++)
        f->f_localsplus[i] = NULL;

**ob_size** 是 code->co_stacksize, code->co_nlocals, code->co_cellvars 和 code->co_freevars 的和

**code->co_stacksize**: 一个整型, 表示函数执行时会使用到的最大的堆栈空间, 这个值是 **code** 对象生成时预先计算好的

**code->co_nlocals**: 局部变量的数量

**code->co_cellvars**: 一个元组, 包含了自己用到并且内嵌函数也用到的所有的变量名称

**code->nfrees**: 自己是内嵌函数的情况下, 包含了外部函数和自己同时用到的变量名称

更多关于 **PyCodeObject** 的信息请参考 [What is a code object in Python?](https://www.quora.com/What-is-a-code-object-in-Python) 和 [code 对象](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/code/code_cn.md)

我们来看一个示例

    def g2(a, b=1, c=2):
        yield a
        c = str(b + c)
        yield c
        new_g = range(3)
        yield from new_g

**dis** 的结果如下

	  # ./python.exe -m dis frame_dis.py
      1           0 LOAD_CONST               5 ((1, 2))
                  2 LOAD_CONST               2 (<code object g2 at 0x10c495030, file "frame_dis.py", line 1>)
                  4 LOAD_CONST               3 ('g2')
                  6 MAKE_FUNCTION            1 (defaults)
                  8 STORE_NAME               0 (g2)
                 10 LOAD_CONST               4 (None)
                 12 RETURN_VALUE

    Disassembly of <code object g2 at 0x10c495030, file "frame_dis.py", line 1>:
      2           0 LOAD_FAST                0 (a)
                  2 YIELD_VALUE
                  4 POP_TOP

      3           6 LOAD_GLOBAL              0 (str)
                  8 LOAD_FAST                1 (b)
                 10 LOAD_FAST                2 (c)
                 12 BINARY_ADD
                 14 CALL_FUNCTION            1
                 16 STORE_FAST               2 (c)

      4          18 LOAD_FAST                2 (c)
                 20 YIELD_VALUE
                 22 POP_TOP

      5          24 LOAD_GLOBAL              1 (range)
                 26 LOAD_CONST               1 (3)
                 28 CALL_FUNCTION            1
                 30 STORE_FAST               3 (new_g)

      6          32 LOAD_FAST                3 (new_g)
                 34 GET_YIELD_FROM_ITER
                 36 LOAD_CONST               0 (None)
                 38 YIELD_FROM
                 40 POP_TOP
                 42 LOAD_CONST               0 (None)
                 44 RETURN_VALUE

我们来迭代一遍这个迭代器

	>>> gg = g2("param a")

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example0.png)

第一次 **next** 返回时, **opcode** `0 LOAD_FAST                0 (a)` 已经被执行了, 并且当前的执行位置是在 `2 YIELD_VALUE` 中

字段 **f_lasti** 的值为 2, 表示 python 虚拟机当前的 program counter 在 `2 YIELD_VALUE` 这个位置

**opcode** `LOAD_FAST` 会把对应的参数推到堆 **f_valuestack** 中, 并且 **opcode** `YIELD_VALUE` 会弹出 **f_valuestack** 顶的元素, **pop** 的定义如下 `#define BASIC_POP()       (*--stack_pointer)`

**f_stacktop** 中的值和前一幅图的值相同, 但是 **f_valuestack** 由于入栈和出栈(出栈并不清空当前的格子)的原因, 里面存储的值已经不为空了

    >>> next(gg)
    'param a'

![example1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1.png)

    >>> next(gg)
    '3'


在第三行代码的 opcode `6 LOAD_GLOBAL              0 (str)` `8 LOAD_FAST                1 (b)` 和 `10 LOAD_FAST                2 (c)` 分别把 **str**(str 存储在了 frame-f_code->co_names 这个字段中), **b**(int 1) 和 **c**(int 2) 推入 **f_valuestack**, opcode `12 BINARY_ADD` 弹出 **f_valuestack**(**b** and **c**) 顶部的两个元素, 相加之后存储回 **f_valuestack** 的顶部, 下图的 **f_valuestack** 为 `12 BINARY_ADD` 执行之后的样子

![example1_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2.png)

opcode `14 CALL_FUNCTION            1` 会弹出可执行对象和可执行对象对应的参数, 并用这些参数传递给可执行对象, 之后执行

执行完成之后, 执行结果 `'3'` 会被压回堆中

![example1_2_1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_1.png)

opcode `16 STORE_FAST               2 (c)` 弹出 **f_valuestack** 顶部的元素, 并把它存储到了 **f_localsplus** 下标为 2 的位置中

![example1_2_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_2_2.png)

opcode `18 LOAD_FAST                2 (c)` 把 **f_localsplus** 位置下标为 2 的元素推入 **f_valuestack**, 之后  `20 YIELD_VALUE ` 弹出这个元素并把它传递给调用者

字段 **f_lasti** 位置的值为 20, 表明当前正在 opcode `20 YIELD_VALUE` 的位置

![example2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example2.png)

在 `24 LOAD_GLOBAL              1 (range)` 和 `26 LOAD_CONST               1 (3)` 之后

![example1_3_1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_1.png)

在 `28 CALL_FUNCTION            1` 之后

![example1_3_2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_2.png)

在 `30 STORE_FAST               3 (new_g)` 之后

![example1_3_3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_3.png)

在 `32 LOAD_FAST                3 (new_g)` 之后

![example1_3_4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example1_3_4.png)

 opcode `34 GET_YIELD_FROM_ITER` 作用是保证堆顶的元素是一个可迭代对象

`36 LOAD_CONST               0 (None)` 把 `None` 推到了堆中

    >>> next(gg)
    0

字段 **f_lasti** 现在值是 36, 表明他在 `38 YIELD_FROM` 之前

![example3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/example3.png)

栈帧对象在 **StopIteration** 抛出后就进入了释放阶段(opcode `44 RETURN_VALUE` 执行之后)

    >>> next(gg)
    1
    >>> next(gg)
    2
    >>> next(gg)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration
    >>> repr(gg.gi_frame)
    'None'

##### f_blockstack

f_blockstack 是一个数组, 里面的元素的类型是 **PyTryBlock**, 数组大小为 **CO_MAXBLOCKS**(20)

这是 **PyTryBlock** 的定义

    typedef struct {
        int b_type;                 /* block 类型 */
        int b_handler;              /* block 处理机制的位置 */
        int b_level;                /* 堆位置 */
    } PyTryBlock;

我们来定义一个有许多 block 的迭代器

    def g3():
        try:
            yield 1
            1 / 0
        except ZeroDivisionError:
            yield 2
            try:
                yield 3
                import no
            except ModuleNotFoundError:
                for i in range(3):
                    yield i + 4
                yield 4
            finally:
                yield 100


	>>> gg = g3()

![blockstack0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack0.png)

在第一个 **yield** 声明时, 第一个 **try block** 已经设置好了

**f_iblock** 值为 1, 表明当前只有一个 block

**b_type** 122 是 opcode `SETUP_FINALLY` 的值, **b_handler** 20 是 `except ZeroDivisionError` 这个opcode 的位置, **b_level** 0 是即将使用的堆的位置

    >>> next(gg)
    1

![blockstack1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack1.png)

**b_type** 257 是 opcode `EXCEPT_HANDLER` 的值, opcode `EXCEPT_HANDLER` 有特殊的含义

    /* EXCEPT_HANDLER is a special, implicit block type which is created when
       entering an except handler. It is not an opcode but we define it here
       as we want it to be available to both frameobject.c and ceval.c, while
       remaining private.*/
    /* 翻译一下: EXCEPT_HANDLER 是一个特殊的 opcode,
    表示一个 try block 已经进入对应的处理机制, 他实际上不是一个传统意义上的 opcode,
    我们在这里定义这个值是为了 frameobject.c 和 ceval.c 能引用他 */
    #define EXCEPT_HANDLER 257

**b_handler** 值为 -1, 表示当前的 try block 已经在处理中

**b_level** 的值没有改变

    >>> next(gg)
    2

![blockstack2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack2.png)

**f_iblock** 值为 3, 第二个 try block 来自 `finally:`(opcode 位置 116), 第三个来自 `except ModuleNotFoundError:`(opcode 位置 62)

    >>> next(gg)
    3

![blockstack3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack3.png)

    >>> next(gg)
    4

第三个 try block 的 **b_type**  变为了 257 并且 **b_handler** 变为 -1, 表明当前的 block 正在处理中

![blockstack4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack4.png)

另外两个 try block 也正确的处理完了

    >>> next(gg)
    5
    >>> next(gg)
    6
    >>> next(gg)
    4
    >>> next(gg)
    100

![blockstack5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/blockstack5.png)

frame 对象进入释放阶段

    >>> next(gg)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

##### f_back

**f_back** 是一个指向前一个 frame 的指针, 他把相关联的 frame 对象串联成一个单链表

	import inspect

    def g4(depth):
        print("depth", depth)
        print(repr(inspect.currentframe()), inspect.currentframe().f_back)
        if depth > 0:
            g4(depth-1)


    g4(3)

输出

    depth 3
    <frame at 0x7fedc2f2e9a8, file '<input>', line 3, code g4> <frame at 0x7fedc2cab468, file '<input>', line 1, code <module>>
    depth 2
    <frame at 0x7fedc2de54a8, file '<input>', line 3, code g4> <frame at 0x7fedc2f2e9a8, file '<input>', line 5, code g4>
    depth 1
    <frame at 0x7fedc2ca6348, file '<input>', line 3, code g4> <frame at 0x7fedc2de54a8, file '<input>', line 5, code g4>
    depth 0
    <frame at 0x10c2c9930, file '<input>', line 3, code g4> <frame at 0x7fedc2ca6348, file '<input>', line 5, code g4>

![f_back](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/f_back.png)

#### free_list 机制

##### zombie frame

第一次 code 对象和一个 frame 对象绑定时, 在这段代码段执行完成后, frame 对象不会被释放, 它会进入一个 "zombie" frame 状态, 下一次同个代码段执行时, 这个 frame 对象会优先被复用

这个策略可以节省 malloc/realloc 的开销, 也可以避免某些字段/值的重复的初始化

    def g5():
        yield 1

    >>> gg = g5()
    >>> gg.gi_frame
    <frame at 0x10224c970, file '<stdin>', line 1, code g5>
    >>> next(gg)
    1
    >>> next(gg)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

    >>> gg3 = g5()
    >>> gg3.gi_frame # id s和之前的对象相同, 同样的 frame 对象在同一个 code block 中复用了
    <frame at 0x10224c970, file '<stdin>', line 1, code g5>

##### free_list sub

有一个单链表存储了部分即将进入回收状态的 frame 对象, 这个机制也可以节省 malloc/free 开销

    static PyFrameObject *free_list = NULL;
    static int numfree = 0;         /* number of frames currently in free_list */
    /* max value for numfree */
    #define PyFrame_MAXFREELIST 200

当一个 **PyFrameObject** 对象在 free_list 上时, 只有下面几个字段的值是有意义的

    ob_type             == &Frametype
    f_back              next item on free list, or NULL
    f_stacksize         size of value stack
    ob_size             size of localsplus

如果是从 free_list 中获取到的 frame 对象, 创建的函数会检测这个取出的 frame 是否有足够的堆空间

    if (Py_SIZE(f) < extras) {
        PyFrameObject *new_f = PyObject_GC_Resize(PyFrameObject, f, extras);

我们来看一个示例

    import inspect

    def g6():
        yield repr(inspect.currentframe()), inspect.currentframe().f_back

    >>> gg = g6()
    >>> gg1 = g6()
    >>> gg2 = g6()

![free_list0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list0.png)

和 **gg** 对象关联的 frame 进入了回收阶段, 因为当前的 **code** 对象 "zombie" frame 字段为空, 所以这个 frame 成为了 **code** 对象的 "zombie" frame

这个 frame 不会进入到 free_list 或者 gc 阶段(**code** 还持有着这个 frame 对象的引用计数 "zombie" frame)

    >>> next(gg)
    ("<frame at 0x1052d83a0, file '<stdin>', line 2, code g6>", <frame at 0x105225e50, file '<stdin>', line 1, code <module>>)
    >>> next(gg)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![free_list1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list1.png)

    >>> next(gg1)
    ("<frame at 0x105620040, file '<stdin>', line 2, code g6>", <frame at 0x105474cc0, file '<stdin>', line 1, code <module>>)
    >>> next(gg1)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![free_list2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list2.png)

    >>> next(gg2)
	("<frame at 0x105482d00, file '<stdin>', line 2, code g6>", <frame at 0x105225e50, file '<stdin>', line 1, code <module>>)
    >>> next(gg2)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![free_list3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/free_list3.png)
