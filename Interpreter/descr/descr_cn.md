# descr

感谢 @Hanaasagi 指出之前本章节存在的问题 [#19](https://github.com/zpoint/CPython-Internals/issues/19) 已改正

# 目录

* [相关位置文件](#相关位置文件)
* [python 中的属性访问机制是如何运作的?](#python-中的属性访问机制是如何运作的)
	* [内建类型](#内建类型)
        * [实例属性访问](#实例属性访问)
        * [类属性访问](#类属性访问)
	* [自定义类型](#自定义类型)
        * [自定义类型的实例属性访问](#自定义类型的实例属性访问)
        * [自定义类型的类属性访问](#自定义类型的类属性访问)
* [method_descriptor](#method_descriptor)
	* [内存构造](#内存构造)
* [如何更改属性访问机制的行为?](#如何更改属性访问机制的行为)
* [相关阅读](#相关阅读)

# 相关位置文件

* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h
* cpython/Objects/object.c
* cpython/Include/object.h
* cpython/Objects/typeobject.c
* cpython/Include/cpython/object.h

# python 中的属性访问机制是如何运作的

在查看 descriptor 对象是如何实现之前我们先来看一个示例

	print(type(str.center)) # <class 'method_descriptor'>
    print(type("str".center)) # <class 'builtin_function_or_method'>

**method_descriptor** 是什么类型 ? 为什么 `str.center` 返回的类型是 **method_descriptor** 而 `"str".center` 返回的类型是 **builtin_function_or_method** ? python 中的属性访问机制是如何运作的 ?

## 内建类型

### 实例属性访问

这是 `inspect.ismethoddescriptor` 和 `inspect.isdatadescriptor` 的定义

    def ismethoddescriptor(object):
        if isclass(object) or ismethod(object) or isfunction(object):
            # 这些条件是要互斥的
            return False
        tp = type(object)
        return hasattr(tp, "__get__") and not hasattr(tp, "__set__")

    def isdatadescriptor(object):
        """如果对象是 data descriptor 则返回 true

        Data descriptors 同时拥有 __get__ 和 a __set__ 这两个属性.  比如某些对象, 在 python 中只定义了属性但是, 但是 getsets 和 成员变量都在 C 中定义的这类对象
        通常来讲, data descriptors 也会有 __name__ 和 __doc__ 这两个属性, 但是不保证百分百的都有"""
        if isclass(object) or ismethod(object) or isfunction(object):
            # 这些条件是要互斥的
            return False
        tp = type(object)
        return hasattr(tp, "__set__") and hasattr(tp, "__get__")

通过上面的代码和注释, 我们知道 **data descriptor** 同时定义了 `__set__` and `__get__` 属性, 而 **method descriptor** 只定义了 `__get__` 属性

(同时也知道定义了 `__get__` 和 `__set__` 的类型的示例被叫做 Data descriptors, 有关 Data descriptors 的资料请参考 [相关阅读](#相关阅读), 下面的内容主要是梳理上面提到的问题)

如果我们用 **dis** 模块处理一个只包含这条命令的脚本 `print(type(str.center))`

	./python.exe -m dis test.py
      1           0 LOAD_NAME                0 (print)
                  2 LOAD_NAME                1 (type)
                  4 LOAD_NAME                2 (str)
                  6 LOAD_ATTR                3 (center)
                  8 CALL_FUNCTION            1
                 10 CALL_FUNCTION            1
                 12 POP_TOP
                 14 LOAD_CONST               0 (None)
                 16 RETURN_VALUE

我们可以看到核心的 **opcode** 就是 `LOAD_ATTR`, 到 `Python/ceval.c` 中可以找到 `LOAD_ATTR` 的如下定义

    case TARGET(LOAD_ATTR): {
        PyObject *name = GETITEM(names, oparg);
        PyObject *owner = TOP();
        PyObject *res = PyObject_GetAttr(owner, name);
        Py_DECREF(owner);
        SET_TOP(res);
        if (res == NULL)
            goto error;
        DISPATCH();
    }
    //  在 Objects/object.c 中可以找到 PyObject_GetAttr 的定义
    PyObject *
    PyObject_GetAttr(PyObject *v, PyObject *name)
    {
        PyTypeObject *tp = Py_TYPE(v);

        if (!PyUnicode_Check(name)) {
            PyErr_Format(PyExc_TypeError,
                         "attribute name must be string, not '%.200s'",
                         name->ob_type->tp_name);
            return NULL;
        }
        /* 首先尝试调用 C 结构体中定义的 tp_getattro 字段/位置方法 */
        if (tp->tp_getattro != NULL)
            return (*tp->tp_getattro)(v, name);
        /* 如果没有定义 tp_getattro, 则尝试调用 tp_getattr 方法 */
        if (tp->tp_getattr != NULL) {
            const char *name_str = PyUnicode_AsUTF8(name);
            if (name_str == NULL)
                return NULL;
            return (*tp->tp_getattr)(v, (char *)name_str);
        }
        /* 如果连 tp_getattr 也没有定义, 则抛出异常 */
        PyErr_Format(PyExc_AttributeError,
                     "'%.50s' object has no attribute '%U'",
                     tp->tp_name, name);
        return NULL;
    }

[**tp_getattro**](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_getattro) 在文档中描述如下

> 一个可选的指向获取属性的函数的指针

这个函数接受两个参数, `PyObject *o, PyObject *attr_name`

[**tp_getattr**](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_getattr) 在文档中描述如下

> 一个可选的指向获取字符串属性的函数的指针
> 这个字段已经弃用了, 当这个字段指向了定义的函数时, 它应该指向一个功能和 tp_getattro 相同的函数, 但是第二个作为属性名称的参数应该是一个 C 字符串而不是 Python 字符串

这个函数接受两个参数, `PyObject *o, char *attr_name`

他们唯一的区别就是第二个参数, **tp_getattro** 接受 `PyObject *` 作为第二个参数, **tp_getattr** 接受 `char *` 作为第二个参数

并且从前面的 `PyObject_GetAttr` 函数, 我们可以发现 `tp_getattro` 比起 `tp_getattr` 有更高的优先级

我们可以在 `Objects/unicodeobject.c` 找到 **str** 这个类型的定义

    PyTypeObject PyUnicode_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "str",                        /* tp_name */
        sizeof(PyUnicodeObject),      /* tp_basicsize */
        0,                            /* tp_itemsize */
        /* Slots */
        (destructor)unicode_dealloc,  /* tp_dealloc */
        0,                            /* tp_print */
        0,                            /* tp_getattr */
        0,                            /* tp_setattr */
        0,                            /* tp_reserved */
        unicode_repr,                 /* tp_repr */
        &unicode_as_number,           /* tp_as_number */
        &unicode_as_sequence,         /* tp_as_sequence */
        &unicode_as_mapping,          /* tp_as_mapping */
        (hashfunc) unicode_hash,      /* tp_hash*/
        0,                            /* tp_call*/
        (reprfunc) unicode_str,       /* tp_str */
        PyObject_GenericGetAttr,      /* tp_getattro */
        0,                            /* tp_setattro */
        ...

我们可以发现, 它使用了一个通用的默认方法 `PyObject_GenericGetAttr` 放在它的 `tp_getattro` 字段中, 我们在 `Objects/object.c` 中可以看到这个方法的定义

    PyObject *
    PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
    {
        return _PyObject_GenericGetAttrWithDict(obj, name, NULL, 0);
    }

    PyObject *
    _PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                     PyObject *dict, int suppress)
    {
        PyTypeObject *tp = Py_TYPE(obj);
        PyObject *descr = NULL;
        PyObject *res = NULL;
        descrgetfunc f;
        Py_ssize_t dictoffset;
        PyObject **dictptr;

        if (!PyUnicode_Check(name)){
            PyErr_Format(PyExc_TypeError,
                         "attribute name must be string, not '%.200s'",
                         name->ob_type->tp_name);
            return NULL;
        }
        Py_INCREF(name);

        if (tp->tp_dict == NULL) {
            if (PyType_Ready(tp) < 0)
                goto done;
        }
        /* 通过 MRO 搜索这个名称对应的属性 */
        descr = _PyType_Lookup(tp, name);

        f = NULL;
        if (descr != NULL) {
            /* 通过 MRO 搜索到了对应名称的变量 */
            Py_INCREF(descr);
            /* 获取到 descr 对象对应的 tp_descr_get 字段, 在 python 中这个字段叫做 __get__ */
            f = descr->ob_type->tp_descr_get;
            /* 检查这个对象是否是 data descriptor, PyDescr_IsData 的定义是  #define PyDescr_IsData(d) (Py_TYPE(d)->tp_descr_set != NULL) */
            if (f != NULL && PyDescr_IsData(descr)) {
            	/* 检查通过, 这个对象同时包含了 __get__ 和 __set__ 属性 */
                /* 如果这个对象是 data descriptor, 尝试调用它的 __get__ 方法并把返回的东西存储在 res 中  */
                res = f(descr, obj, (PyObject *)obj->ob_type);
                if (res == NULL && suppress &&
                        PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    PyErr_Clear();
                }
                /* 完成了 */
                goto done;
            }
        }
        /* 执行到了这里, descr 为 NULL 指针(表示通过 MRO 无法找到对应名称的对象) 或者 descr 对象未定义 __set__ 属性 */
        if (dict == NULL) {
        	/* 尝试找到对象中的 __dict__ 属性 */
            dictoffset = tp->tp_dictoffset;
            if (dictoffset != 0) {
                if (dictoffset < 0) {
                    Py_ssize_t tsize;
                    size_t size;

                    tsize = ((PyVarObject *)obj)->ob_size;
                    if (tsize < 0)
                        tsize = -tsize;
                    size = _PyObject_VAR_SIZE(tp, tsize);
                    _PyObject_ASSERT(obj, size <= PY_SSIZE_T_MAX);

                    dictoffset += (Py_ssize_t)size;
                    _PyObject_ASSERT(obj, dictoffset > 0);
                    _PyObject_ASSERT(obj, dictoffset % SIZEOF_VOID_P == 0);
                }
                dictptr = (PyObject **) ((char *)obj + dictoffset);
                dict = *dictptr;
            }
        }
        if (dict != NULL) {
        /* 如果找到了 __dict__ 属性, 尝试去 __dict__ 这个字典中搜索这个名称对应的值 */
            Py_INCREF(dict);
            res = PyDict_GetItem(dict, name);
            if (res != NULL) {
                Py_INCREF(res);
                Py_DECREF(dict);
                goto done;
            }
            Py_DECREF(dict);
        }
        /* 运行到了这里, 要么 __dict__ 对象为空指针, 或者对应的名称未存储在 __dict__ 中, 或者 descr 未定义 __set__ 属性 */
        if (f != NULL) {
            /* 定义了 __get__ 但是未定义 __set__ */
            /* 尝试调用 __get__ 方法 */
            res = f(descr, obj, (PyObject *)Py_TYPE(obj));
            if (res == NULL && suppress &&
                    PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
            }
            goto done;
        }
        /* 到了这里, descr 对象没有定义 __get__ 方法, 并且也没有办法在 __dict__ 中找到对应的名称 */
        if (descr != NULL) {
        	/* 如果在 MRO 中找到了对应的名称的对象, 返回这个对象 */
            res = descr;
            descr = NULL;
            goto done;
        }

        if (!suppress) {
            PyErr_Format(PyExc_AttributeError,
                         "'%.50s' object has no attribute '%U'",
                         tp->tp_name, name);
        }
      done:
        Py_XDECREF(descr);
        Py_DECREF(name);
        return res;
    }

我们可以根据上面的代码内容画出整个流程

![_str__attribute_access](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/_str__attribute_access.png)

到了这里, 我发现我弄错了一个地方, 执行命令 `str.center` 的时候, `PyUnicode_Type` 中的 `tp_getattro`实际上是不会被调用的. 相反, 他们会在命令 `"str".center` 的过程中被调用, 下面的代码可以在表面看到这两个方式的不同

    >>> type("str")
    <class 'str'>
    >>> "str".center # 调用了 PyUnicode_Type 中 tp_getattro 存储的函数
    <built-in method center of str object at 0x10360f500>
    >>> type("str".center)
	<class 'builtin_function_or_method'>

    >>> type(str)
    <class 'type'>
    >>> str.center # 调用了 PyType_Type 中 tp_getattro 存储的函数
    <method 'center' of 'str' objects>
    >>> type(str.center)
	<class 'method_descriptor'>

所以, 到这里之前上面讲的所有过程都是 `"str".center` 这个实例的属性访问

### 类属性访问

我们来找找 `<class 'type'>` 的定义, 还有 `str.center` 的属性访问是如何工作的(大致上和 `"str".center` 相同)

对于类型 `<class 'type'>`, opcode `LOAD_ATTR` 依然会调用 `type_getattro` 字段中存储的函数

    PyTypeObject PyType_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "type",                                     /* tp_name */
        sizeof(PyHeapTypeObject),                   /* tp_basicsize */
        sizeof(PyMemberDef),                        /* tp_itemsize */
        (destructor)type_dealloc,                   /* tp_dealloc */
        0,                                          /* tp_print */
        0,                                          /* tp_getattr */
        0,                                          /* tp_setattr */
        0,                                          /* tp_reserved */
        (reprfunc)type_repr,                        /* tp_repr */
        0,                                          /* tp_as_number */
        0,                                          /* tp_as_sequence */
        0,                                          /* tp_as_mapping */
        0,                                          /* tp_hash */
        (ternaryfunc)type_call,                     /* tp_call */
        0,                                          /* tp_str */
        (getattrofunc)type_getattro,                /* tp_getattro */
        (setattrofunc)type_setattro,                /* tp_setattro */
        0,                                          /* tp_as_buffer */
        ...
    }

    static PyObject *
    type_getattro(PyTypeObject *type, PyObject *name)
    {
    	/*
        	这个函数的逻辑和 _PyObject_GenericGetAttrWithDict 大致上相同,
            对于这部分有兴趣的同学, 请直接参考 Objects/typeobject.c
        */
    }

![str_attribute_access](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/str_attribute_access.png)

从上面的图片和代码段, 我们可以知道, 类 `str` 和实例 `"str"` 的 `tp_getattro` 函数是不同的, 但是在他们各自的函数路径中, 最后都会调用到同一个类型为 `method_descriptor` 的对象的 `tp_descr_get` 中的函数, 并把这个函数返回的东西返回给调用着

	/* 位置在 cpython/Objects/descrobject.c */
    /* 这个函数就是 method_descriptor 对象的  tp_descr_get 中存储的函数 */
    static PyObject *
    method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
    {
        PyObject *res;
        /* descr_check 检查 descr 对象是通过自己或者自己的继承类往上的对象中找到的 */
        /* 直接翻译有点绕口, 和 cpython 内部的实现有关, 通俗的讲, 和 inspect.isclass() 作用类似 */
        if (descr_check((PyDescrObject *)descr, obj, &res))
        	/* str.center 会进到这里, 这里返回的对象类型是 PyMethodDescrObject */
            return res;
        /* "str".center 会进到这里, 返回一个 PyCFunction 对象, 关于 PyCFunction 可以返回到外面目录并点击 function */
        return PyCFunction_NewEx(descr->d_method, obj, NULL);
    }

好了, 现在我们已经找到下面的问题的答案了
* 为什么 `str.center` 返回的类型是 **method_descriptor** 而 `"str".center` 返回的类型是 **builtin_function_or_method** ?
* python 中的属性访问机制是如何运作的 ?

## 自定义类型

假设你已经了解了如下两个过程

* [属性在类/实例创建时是如何初始化的](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/slot_cn.md#%E6%B2%A1%E6%9C%89slots)
* [类/实例的创建过程](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type_cn.md#class-%E7%9A%84%E5%88%9B%E5%BB%BA)

### 自定义类型的实例属性访问

通过以上的分析, 我们可以知道关键的部分就是 `type(instance)` 中字段 `tp_getattro` 所存储的值, 对于内建类型 `tp_getattro` 字段所存储的函数都是预先在 C 中定义好的, 然而用户创建的类型却是在解释器运行时在创建类的过程中给 `tp_getattro` 这个字段赋上的一个值, 在这个过程中, 肯定有某个函数的指针被存储到了新创建对象的 `tp_getattro` 字段中

    class A(object):
        def __getattr__(self, item):
            pass


    class B(object):
        def __getattribute__(self, item):
            pass


    class C(object):
        pass

`class A`, `class B` 和 `class C` 中 `tp_getattro` 字段存储的值是什么呢 ?

下面是类创建过程中的部分代码段

    /* cpython/Objects/typeobject.c */
    static slotdef slotdefs[] = {
        /* ... */
        TPSLOT("__getattribute__", tp_getattro, slot_tp_getattr_hook,
        wrap_binaryfunc,
        "__getattribute__($self, name, /)\n--\n\nReturn getattr(self, name)."),
        TPSLOT("__getattr__", tp_getattro, slot_tp_getattr_hook, NULL, ""),
        TPSLOT("__setattr__", tp_setattro, slot_tp_setattro, wrap_setattr,
        /* ... */
    }

    static PyObject *
    type_new(PyTypeObject *metatype, PyObject *args, PyObject *kwds)
    {
            /* ... */
            /* 把对应的 slots 放到对应的位置上 */
            fixup_slot_dispatchers(type);
            /* ... */
    }

    /* 在创建 class (type) 时, 根据用户覆写的函数情况, 把对应的函数存储到对应的位置上 */
    static void
    fixup_slot_dispatchers(PyTypeObject *type)
    {
        slotdef *p;

        init_slotdefs();
        for (p = slotdefs; p->name; )
            p = update_one_slot(type, p);
    }

    static slotdef *
    update_one_slot(PyTypeObject *type, slotdef *p)
    {
        PyObject *descr;
        PyWrapperDescrObject *d;
        void *generic = NULL, *specific = NULL;
        int use_generic = 0;
        int offset = p->offset;
        int error;
        void **ptr = slotptr(type, offset);

        if (ptr == NULL) {
            do {
                ++p;
            } while (p->offset == offset);
            return p;
        }
        assert(!PyErr_Occurred());
        do {
            descr = find_name_in_mro(type, p->name_strobj, &error);
            if (descr == NULL) {
                if (error == -1) {
                    PyErr_Clear();
                }
                if (ptr == (void**)&type->tp_iternext) {
                    specific = (void *)_PyObject_NextNotImplemented;
                }
                continue;
            }
            if (Py_TYPE(descr) == &PyWrapperDescr_Type &&
                ((PyWrapperDescrObject *)descr)->d_base->name_strobj == p->name_strobj) {
                void **tptr = resolve_slotdups(type, p->name_strobj);
                if (tptr == NULL || tptr == ptr)
                    generic = p->function;
                d = (PyWrapperDescrObject *)descr;
                if (d->d_base->wrapper == p->wrapper &&
                PyType_IsSubtype(type, PyDescr_TYPE(d)))
                {
                    if (specific == NULL ||
                        specific == d->d_wrapped)
                        specific = d->d_wrapped;
                    else
                        use_generic = 1;
                }
            }
            else if (Py_TYPE(descr) == &PyCFunction_Type &&
                     PyCFunction_GET_FUNCTION(descr) ==
                     (PyCFunction)(void(*)(void))tp_new_wrapper &&
                     ptr == (void**)&type->tp_new)
            {
                /* ... */
                specific = (void *)type->tp_new;
                /* ... */
            }
            else if (descr == Py_None &&
                     ptr == (void**)&type->tp_hash) {
                /* ... */
                specific = (void *)PyObject_HashNotImplemented;
                /* ... */
            }
            else {
                use_generic = 1;
                generic = p->function;
            }
        } while ((++p)->offset == offset);
        if (specific && !use_generic)
            *ptr = specific;
        else
            *ptr = generic;
        return p;
    }

从上面的代码段我们可以发现, `slotdefs` 预先设定了一个类所必须的属性值, 对于每一个属性值, `update_one_slot` 会根据情况把用户覆写的或者系统内置的函数放到新创建的类的对应的位置上

对于每一个新创建的类, `slotdefs`  为 `__getattribute__` 和 `__getattr__` 检测和设置值时候, 他们拥有同样的 offset, 也就是说这两个属性在新创建的类型的 C 结构体中, 他们的存储的位置相同, 后存储的函数指针会覆盖先存储的函数指针

对于 `class A`, 当设置 `__getattribute__` 时, `offset` 为 `144`, `descr` 的值为 `<slot wrapper '__getattribute__' of 'object' objects>`, `PyType_IsSubtype(type, PyDescr_TYPE(d))` 为真, 所以 `specific` 会成为 `d->d_wrapped`, 也就是 `PyObject_GenericGetAttr`. 在下一个 while 循环中, 当为 `__getattr__` 设置值时, `offset` 也是 `144`, 此时 `descr` 的值变成了 `<function A.__getattr__ at 0x1013260c0>`, while 循环中最后的 `else` 会把 `generic` 设置为 `p->function`, 也就是 `slot_tp_getattr_hook`, 此时 while 循环结束, offset `144` 中存储的值为 `slot_tp_getattr_hook`

对于 `class B`, 当设置 `__getattribute__` 时, `offset` 为 `144`, `descr` 的值为 `<function B.__getattribute__ at 0x103ad6140>`,  while 循环中最后的 `else` 会把 `generic` 设置为 `p->function`, 也就是 `slot_tp_getattr_hook`, 在下一个 while 循环中, 当为 `__getattr__` 设置值时, `offset` 也是 `144`, 此时 `descr` 为一个 `null` 指针, 所以会执行到 `continue` 并结束此次 while 循环, offset `144` 中存储的值为 `slot_tp_getattr_hook`

对于 `class C`, 当设置 `__getattribute__` 时, `offset` 为 `144`, `descr` 的值为 `<slot wrapper '__getattribute__' of 'object' objects>`, `PyType_IsSubtype(type, PyDescr_TYPE(d))` 为真, 所以 `specific` 会被设置为 `d->d_wrapped`, 也就是 `PyObject_GenericGetAttr`, 在下一个 while 循环中, 当为 `__getattr__` 设置值时, `offset` 也是 `144`, 此时 `descr` 为一个 `null` 指针, 所以会执行到 `continue` 并结束此次 while 循环, offset `144` 中存储的值为 `PyObject_GenericGetAttr`

`slot_tp_getattr_hook` 的定义如下

    /* python/Objects/typeobject.c */
    static PyObject *
    slot_tp_getattro(PyObject *self, PyObject *name)
    {
        PyObject *stack[1] = {name};
        return call_method(self, &PyId___getattribute__, stack, 1);
    }

    static PyObject *
    slot_tp_getattr_hook(PyObject *self, PyObject *name)
    {
        PyTypeObject *tp = Py_TYPE(self);
        PyObject *getattr, *getattribute, *res;
        _Py_IDENTIFIER(__getattr__);

        getattr = _PyType_LookupId(tp, &PyId___getattr__);
        if (getattr == NULL) {
            /* No __getattr__ hook: use a simpler dispatcher */
            tp->tp_getattro = slot_tp_getattro;
            return slot_tp_getattro(self, name);
        }
        Py_INCREF(getattr);
        getattribute = _PyType_LookupId(tp, &PyId___getattribute__);
        if (getattribute == NULL ||
            (Py_TYPE(getattribute) == &PyWrapperDescr_Type &&
             ((PyWrapperDescrObject *)getattribute)->d_wrapped ==
             (void *)PyObject_GenericGetAttr))
            res = PyObject_GenericGetAttr(self, name);
        else {
            Py_INCREF(getattribute);
            res = call_attribute(self, getattribute, name);
            Py_DECREF(getattribute);
        }
        if (res == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            res = call_attribute(self, getattr, name);
        }
        Py_DECREF(getattr);
        return res;
    }

对于 `slot_tp_getattr_hook`

* 如果不存在 `___getattr__` 方法, `slot_tp_getattr_hook` 会直接调用 `___getattribute__` 方法

* 如果有存在 `__getattr__` 方法, `slot_tp_getattr_hook` 会调用 `___getattribute__`, 如果调用没有结果返回并且抛出了 `PyExc_AttributeError` 异常, 那么再尝试调用 `__getattr__` 方法

所以只要你的自定义类型覆写了 `__getattribute__` 或者 `__getattr__` 其中的一个或者两个函数, 那么创建类的时候 `tp_getattro` 存储的值将会是 `slot_tp_getattr_hook`

当你的自定义类型没有覆写 `__getattribute__` 或者 `__getattr__` 的任意一个函数时, `tp_getattro` 中存储的值和内建类型存储的值相同, 都是 `PyObject_GenericGetAttr`

### 自定义类型的类属性访问

因为 `type(newly_created_class)` 总是会返回 `<class 'type'>`, 并且 `<class 'type'>` 中 `tp_getattro` 字段的值是在 C 中预先定义好的, 无法进行定制化, 所以这里的属性访问行为和 [类属性访问](#类属性访问) 相同


# method_descriptor

我们来尝试找到剩下的问题的答案
* **method_descriptor** 是什么类型 ?

这个类型的定义可以在这个位置找到 `Include/descrobject.h`

## 内存构造

![PyMethodDescrObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/PyMethodDescrObject.png)

我们可以看到当你尝试获取下面的类属性和实例属性时(`"str".center` 和 `str.center`), 他们都获取了同个  **PyMethodDescrObject** 对象, 这个对象是对 **PyMethodDef** 的包装, 如果你对 **PyMethodDef** 感兴趣, 请参考 [method](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method_cn.md)

    >>> str.center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370
    >>> "str".center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370

除了 **PyMethodDescrObject** 还有一些其他的 descriptor 相关的类型

**PyMemberDescrObject**: 包装了 **PyMemberDef** 对象

**PyGetSetDescrObject**: 包装了 **PyGetSetDef** 对象

# 如何更改属性访问机制的行为

我们现在知道了当你尝试访问一个对象的属性时, python 虚拟机会执行以下操作

1. 执行 opcode `LOAD_ATTR`
2. `LOAD_ATTR` 会尝试调用该对象类型的 `tp_getattro` 中存储的方法, 如果成功返回跳转到 4
3. 抛出异常
4. 返回前面函数返回的东西给调用者

默认的 `__getattribute__` 会在类创建的时候设置好, 具体设置的是哪一个函数取决于这个类是否自定义, 用户覆写函数的情况, 通常情况下是 `PyObject_GenericGetAttr`, 它实现了 **descriptor protocol** (我们在前面的部分通过源代码分析了解过了, 你也可以参考 [相关阅读](#相关阅读) 里的内容)

当我们定义一个 python 对象时, 并且我们想更改这个对象的属性访问的行为时该怎么做呢?

我们没有办法更改 opcode `LOAD_ATTR` 的行为, 它是用 C 语言写好的并集成在了 python 虚拟机内

但是我们可以提供自定义的 `__getattribute__` 和 `__getattr__` 来影响创建这个类型的时候, 设置到 `tp_getattro` 字段上的具体的函数指针

注意, 提供你自定义的 `__getattribute__` 可能会破坏 **descriptor protocol**, 通常情况下我们只用提供 `__getattr__` 就够了

    class A(object):
        def __getattribute__(self, item):
            print("in __getattribute__", item)
            if item in ("noA", "noB"):
                raise AttributeError
            return "__getattribute__" + str(item)

        def __getattr__(self, item):
            print("in __getattr__", item)
            if item == "noB":
                raise AttributeError
            return "__getattr__" + str(item)

    >>> a = A()

    >>> a.x
    in __getattribute__ x
    '__getattribute__x'

    >>> a.noA
    in __getattribute__ noA
    in __getattr__ noA
    '__getattr__noA'

    >>> a.noB
    in __getattribute__ noB
    in __getattr__ noB
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
      File "<input>", line 11, in __getattr__
    AttributeError

# 相关阅读
* [descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)
* [The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)