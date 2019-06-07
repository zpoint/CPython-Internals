# class

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [字段](#字段)
	* [im_func](#im_func)
	* [im_self](#im_self)
* [free_list(缓冲池)](#free_list)
* [classmethod 和 staticmethod](#classmethod-和-staticmethod)
	* [classmethod](#classmethod)
	* [staticmethod](#staticmethod)

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
    <class 'method'>

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

#### classmethod 和 staticmethod

我们来定义一个有 **classmethod** 和 **staticmethod** 的对象看看

    class C(object):
        def f1(self, val):
            return val

        @staticmethod
        def fs():
            pass

        @classmethod
        def fc(cls):
            return cls

    >>> c1 = C()
    >>> type(c1.fs)
    <class 'function'>
    >>> type(c1.fc)
    <class 'method'>

##### classmethod

**@classmethod** 装饰器使得 **c1.fc** 的结果仍然为类型 **method** 的对象

**c1.fc** 是另一个  **PyMethodObject** 的实例, 其中的 **im_func** 指向即将调用的函数对象, 而 **im_self** 指向了 `<class '__main__.C'>`

    >>> C
    <class '__main__.C'>

![classmethod](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod.png)

**classmethod** 内部是如何实现的呢 ?

**classmethod** 在 python3 中是一个类型

    typedef struct {
        PyObject_HEAD
        PyObject *cm_callable;
        PyObject *cm_dict;
    } classmethod;

![classmethod1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod1.png)

我们来尝试理解一下

    fc = classmethod(lambda self : self)

    class C(object):
        fc1 = fc

    >>> cc = C()
    >>> type(fc)
    >>> <class 'classmethod'>
    >>> type(cc.fc1)
    >>> <class 'method'>

    >>> fc.__dict__["b"] = "c"
    >>> cc.fc1
    <bound method <lambda> of <class '__main__.C'>>

同样的变量, 通过不用的方式获取得到的结果不同, 这是怎么一回事呢 ?

当你尝试通过实例 cc 访问属性 **fc1** 时, **descriptor protocol** 会通过好几种不同的方式去尝试获得一个结果, 并把这个结果返回给你, 比如
* 调用 C 的 `__getattribute__`
* 判断 `C.__dict__["fc1"]` 是否为 data descriptor ?
	* 是, 返回 `C.__dict__['fc1'].__get__(instance, Class)`
	* 否, 返回 `cc.__dict__['fc1']` if 'fc1' in `cc.__dict__` else
		* `C.__dict__['fc1'].__get__(instance, Class)` if hasattr(`C.__dict__['fc1']`, `__get__`) else `C.__dict__['fc1']`
* 如果上面的步骤都没找到, 调用 `c.__getattr__("fc1")` 返回

![object-attribute-lookup](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses/object-attribute-lookup.png)

有兴趣的同学可以参考这篇博客的这个部分 [object-attribute-lookup](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses/#object-attribute-lookup) 或者 [descr 对象](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr_cn.md)

![classmethod2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod2.png)

由于 **classmethod** 类型同时实现了 `__get__` 和 `__set__`, 所以这个类型/类型的实例为 **data descriptor**, 通过类属性访问 **cc.fc1** 会调用 `fc1.__get__` 函数, 并返回这个函数所返回的对象给调用者

我们可以看看 **classmethod** 类型的 `__get__` 函数的实现

    static PyObject *
    cm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
    {
        classmethod *cm = (classmethod *)self;

        if (cm->cm_callable == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "uninitialized classmethod object");
            return NULL;
        }
        if (type == NULL)
            type = (PyObject *)(Py_TYPE(obj));
        return PyMethod_New(cm->cm_callable, type);
    }

![classmethod_get](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod_get.png)

当你通过 **cc.fc1** 访问属性 **fc1** 时, **descriptor protocol** 会调用上面这个函数, 上面这个函数返回了一个新的创建的 **PyMethodObject** 对象(通过 **PyMethod_New**), 这个 **PyMethodObject** 里面包的的 **im_func** 就是当前 **classmethod** 的 **cm_callable** 里所存储的函数对象(这里是个 lambda)

##### staticmethod

**@staticmethod** 装饰器把 **c1.fs** 的类型更改为了 [function](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func.md)

    >>> type(c1.fs)
    <class 'function'>

![staticmethod](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/staticmethod.png)

    typedef struct {
        PyObject_HEAD
        PyObject *sm_callable;
        PyObject *sm_dict;
    } staticmethod;

这是 **staticmethod** 对象的构造

![staticmethod1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/staticmethod1.png)

    fs = staticmethod(lambda : None)

    class C(object):
        fs1 = fs

    >>> fs.__dict__["a"] = "b"
    >>> cc = C()
    >>> type(fs)
    >>> <class 'staticmethod'>
    >>> type(cc.fs1)
    >>> <class 'function'>

    >>> cc.fs1
    <function <lambda> at 0x1047d9f40>

![staticmethod2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/staticmethod2.png)

同理  **staticmethod** 也是一个 **data descriptor**

我们可以看看 **staticmethod** 类型的 `__get__` 函数的实现

    static PyObject *
    sm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
    {
        staticmethod *sm = (staticmethod *)self;

        if (sm->sm_callable == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "uninitialized staticmethod object");
            return NULL;
        }
        Py_INCREF(sm->sm_callable);
        return sm->sm_callable;
    }

![staticmethod_get](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/staticmethod_get.png)

当你通过 **cc.fs1** 访问属性 **fs1** 时, **descriptor protocol** 再一次的调用了 `C.__dict__["fs1"]__get__(instance, Class)` 并返回了对应的 **lambda** 函数对象
