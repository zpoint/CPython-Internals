# descr

### 目录

* [相关位置文件](#相关位置文件)
* [python 中的属性访问机制是如何运作的?](#python-中的属性访问机制是如何运作的?)
	* [实例属性访问](#实例属性访问)
	* [类属性访问](#类属性访问)
* [method_descriptor](#method_descriptor)
	* [内存构造](#内存构造)
* [相关阅读](#相关阅读)

#### 相关位置文件

* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h
* cpython/Objects/object.c
* cpython/Include/object.h
* cpython/Objects/typeobject.c
* cpython/Include/cpython/object.h

#### python 中的属性访问机制是如何运作的?

在查看 descriptor 对象是如何实现之前我们先来看一个示例

	print(type(str.center)) # <class 'method_descriptor'>
    print(type("str".center)) # <class 'builtin_function_or_method'>

**method_descriptor** 是什么类型 ? 为什么 `str.center` 返回的类型是 **method_descriptor** 而 `"str".center` 返回的类型是 **builtin_function_or_method** ? python 中的属性访问机制是如何运作的 ?

##### 实例属性访问

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

通过上面的代码和注释, 我们知道类型 **method_descriptor** 或者以 **_descriptor** 结尾的类型, 是那类 getsets 和成员变量在 C 中定义的类型

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
        /* f首先尝试调用 __getattribute__ 方法 */
        if (tp->tp_getattro != NULL)
            return (*tp->tp_getattro)(v, name);
        /* 如果 __getattribute__ 失败了, 则尝试调用 __getattr__ 方法 */
        if (tp->tp_getattr != NULL) {
            const char *name_str = PyUnicode_AsUTF8(name);
            if (name_str == NULL)
                return NULL;
            return (*tp->tp_getattr)(v, (char *)name_str);
        }
        PyErr_Format(PyExc_AttributeError,
                     "'%.50s' object has no attribute '%U'",
                     tp->tp_name, name);
        return NULL;
    }

**tp_getattro** 在 c 里面表示 `__getattribute__` 方法

**tp_getattr** 在 c 里面表示 `__getattr__` 方法

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

我们可以发现, 它使用了一个通用的默认方法 `PyObject_GenericGetAttr` 放在它的 `tp_getattro` (` __getattribute__` 在 c 中的名称) 字段中, 我们在 `Objects/object.c` 中可以看到这个方法的定义

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
            /* 检查这个对象是否是 ata descriptor, PyDescr_IsData 的定义是  #define PyDescr_IsData(d) (Py_TYPE(d)->tp_descr_set != NULL) */
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

到了这里, 我发现我弄错了一个地方, 执行命令 `str.center` 的时候, `PyUnicode_Type` 中的 `tp_getattro`(` __getattribute__` 在 c 中的名称) 实际上是不会被调用的. 相反, 他们会在命令 `"str".center` 的过程中被调用, 下面的代码可以在表面看到这两个方式的不同

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

##### 类属性访问

我们来找找 `<class 'type'>` 的定义, 还有 `str.center` 的属性访问是如何工作的(大致上和 `"str".center` 相同)

对于类型 `<class 'type'>`, opcode `LOAD_ATTR` 依然会调用 `type_getattro` 字段中存储的函数(python 中叫做`__getattribute__`)

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

从上面的图片和代码段, 我们可以知道, 类 `str` 和实例 `"str"` 的 `__getattribute__` 函数是不同的, 但是在他们各自的函数路径中, 最后都会调用到同一个类型为 `method_descriptor` 的对象的 `tp_descr_get` 中的函数, 并把这个函数返回的东西返回给调用着

	/* 位置在 cpython/Objects/descrobject.c */
    /* 这个函数就是 method_descriptor 对象的  tp_descr_get 中存储的函数 */
    static PyObject *
    method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
    {
        PyObject *res;
        /* descr_check 检查 descr 对象是通过自己或者自己的继承类往上的对象中找到的 */
        /* 直接翻译有点绕口, 和 cpython 内部的实现有关, 通俗的讲, 和 inspect.isclass() 作用类似
        if (descr_check((PyDescrObject *)descr, obj, &res))
        	/* str.center 会进到这里, 这里返回的对象类型是 PyMethodDescrObject */
            return res;
        /* "str".center 会进到这里, 返回一个 PyCFunction 对象, 关于 PyCFunction 可以返回到外面目录并点击 function */
        return PyCFunction_NewEx(descr->d_method, obj, NULL);
    }

好了, 现在我们已经找到下面的问题的答案了
* 为什么 `str.center` 返回的类型是 **method_descriptor** 而 `"str".center` 返回的类型是 **builtin_function_or_method** ?
* python 中的属性访问机制是如何运作的 ?

#### method_descriptor

我们来尝试找到剩下的问题的答案
* **method_descriptor** 是什么类型 ?

这个类型的定义可以在这个位置找到 `Include/descrobject.h`

##### 内存构造

![PyMethodDescrObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/PyMethodDescrObject.png)

我们可以看到当你尝试获取下面的类属性和实例属性时(`"str".center` 和 `str.center`), 他们都获取了同个  **PyMethodDescrObject** 对象, 这个对象是对 **PyMethodDef** 的包装, 如果你对 **PyMethodDef** 感兴趣, 请参考 [method](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method_cn.md)

    >>> str.center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370
    >>> "str".center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370

除了 **PyMethodDescrObject** 还有一些其他的 descriptor 相关的类型

**PyMemberDescrObject**: 包装了 **PyMemberDef** 对象

**PyGetSetDescrObject**: 包装了 **PyGetSetDef** 对象

#### 相关阅读
* [descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)
[The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)