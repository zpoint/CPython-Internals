# class

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [字段](#字段)
	* [im_func](#im_func)
	* [im_self](#im_self)
* [free_list(缓冲池)](#free_list)

#### 相关位置文件
* cpython/Objects/classobject.c
* cpython/Include/classobject.h

#### 内存构造

**PyMethodObject** 在 c 层级表示了一个 python 层级的 **method** 对象

    class C(object):
        def f1(self, val):
            return val

    >>> c = C()
    >>> type(c.f1)
    method

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/layout.png)

#### 字段

这是 **c.f** 的构造

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/example0.png)

##### im_func

你可以从上图看到, **im_func** 存储的是一个类型为 [function](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func_cn.md) 的对象

    >>> C.f1
    <function C.f1 at 0x10b80f040>

##### im_self

**im_self** 存储了该 method 绑定的实例对象

    >>> c
    <__main__.C object at 0x10b7cbcd0>

当你调用下面的 method 的时候

	>>> c.f1(123)
	123

**PyMethodObject** 把实际的调用指派给了自己所存储的 **im_func** 对象, 在指派的过程中把 **im_self** 作为第一个参数传入

    static PyObject *
    method_call(PyObject *method, PyObject *args, PyObject *kwargs)
    {
        PyObject *self, *func;
		/* 获取 im_self 中存储的对象*/
        self = PyMethod_GET_SELF(method);
        if (self == NULL) {
            PyErr_BadInternalCall();
            return NULL;
        }
		/* 获取 im_func 中存储的对象*/
        func = PyMethod_GET_FUNCTION(method);
		/* 调用 im_func, 调用过程中把 im_self 作为第一个参数传入 */
        return _PyObject_Call_Prepend(func, self, args, kwargs);
    }

#### free_list

    static PyMethodObject *free_list;
    static int numfree = 0;
    #ifndef PyMethod_MAXFREELIST
    #define PyMethod_MAXFREELIST 256
    #endif

free_list 是一个单链表, 作缓冲池用, 用来减小 **PyMethodObject** 这个对象的 malloc/free 的开销

**im_self** 这个字段被用来连接链表的元素

**PyMethodObject** 只会在你想要获取一个 bound-method 的时候创建, 而不是这个类实例化的时候创建

    >>> c1 = C()
    >>> id(c1)
    4514815184
    >>> c2 = C()
    >>> id(c2)
    4514815472
    >>> id(c1.f1) # c1.f1 是在这一行创建的, 在这一行之后, c1.f1 的引用计数器变为 0, 将会被回收
    4513259240
    >>> id(c1.f1) # c1.f1 的 id 被循环利用了
    4513259240
    >>> id(c2.f1)
    4513259240

现在我们来看一个 free_list 的例子

	>>> c1_f1_1 = c1.f1
	>>> c1_f1_2 = c1.f1
    >>> id(c1_f1_1)
    4529762024
    >>> id(c1_f1_2)
    4529849392

假设 free_list 在当前状态下是空的

![free_list0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list0.png)

    >>> del c1_f1_1
    >>> del c1_f1_2

![free_list1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list1.png)

    >>> c1_f1_3 = c1.f1
    >>> id(c1_f1_3)
    4529849392

![free_list2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list2.png)