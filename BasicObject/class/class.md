# class

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [fields](#fields)
    * [im_func](#im_func)
    * [im_self](#im_self)
* [free_list](#free_list)
* [classmethod and staticmethod](#classmethod-and-staticmethod)
    * [classmethod](#classmethod)
    * [staticmethod](#staticmethod)

#### related file
* cpython/Objects/classobject.c
* cpython/Include/classobject.h

#### memory layout

the **PyMethodObject** represents the type **method** in c-level

    class C(object):
        def f1(self, val):
            return val

    >>> c = C()
    >>> type(c.f1)
    <class 'method'>

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/layout.png)

#### fields

the layout of **c.f1**

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/example0.png)

##### im_func

as you can see from the layout, field **im_func** stores the [function](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func.md) object that implements the method

    >>> C.f1
    <function C.f1 at 0x10b80f040>

##### im_self

field **im_self** stores the instance object this method bound to

    >>> c
    <__main__.C object at 0x10b7cbcd0>

when you call

    >>> c.f1(123)
    123

the **PyMethodObject** delegate the real call to **im_func** with **im_self** as the first argument

    static PyObject *
    method_call(PyObject *method, PyObject *args, PyObject *kwargs)
    {
        PyObject *self, *func;
        /* get im_self */
        self = PyMethod_GET_SELF(method);
        if (self == NULL) {
            PyErr_BadInternalCall();
            return NULL;
        }
        /* get im_func */
        func = PyMethod_GET_FUNCTION(method);
        /* call im_func with im_self as the first argument */
        return _PyObject_Call_Prepend(func, self, args, kwargs);
    }

#### free_list

    static PyMethodObject *free_list;
    static int numfree = 0;
    #ifndef PyMethod_MAXFREELIST
    #define PyMethod_MAXFREELIST 256
    #endif

free_list is a single linked list, it's used for **PyMethodObject** to safe malloc/free overhead

**im_self** field is used to chain the element

the **PyMethodObject** will be created when you trying to access the bound-method, not when the instance is created

    >>> c1 = C()
    >>> id(c1)
    4514815184
    >>> c2 = C()
    >>> id(c2)
    4514815472
    >>> id(c1.f1) # c1.f1 is created in this line, after this line, the reference count of c1.f1 becomes 0 and c1.f1 deallocated
    4513259240
    >>> id(c1.f1) # the id is resued
    4513259240
    >>> id(c2.f1)
    4513259240

now, let's see an example of free_list

    >>> c1_f1_1 = c1.f1
    >>> c1_f1_2 = c1.f1
    >>> id(c1_f1_1)
    4529762024
    >>> id(c1_f1_2)
    4529849392

assume the free_list is empty now

![free_list0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list0.png)

    >>> del c1_f1_1
    >>> del c1_f1_2

![free_list1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list1.png)

    >>> c1_f1_3 = c1.f1
    >>> id(c1_f1_3)
    4529849392

![free_list2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/free_list2.png)

#### classmethod and staticmethod

let's define an object with **classmethod** and **staticmethod**

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

the **@classmethod** keeps type of **c1.fc** as **method**

**c1.fc** is another instance of  **PyMethodObject**, with **im_func** bind to the actual callable object, and **im_self** bind to the `<class '__main__.C'>`

    >>> C
    <class '__main__.C'>

![classmethod](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod.png)

how **classmethod** work internally?

**classmethod** is a **type** in python3

    typedef struct {
        PyObject_HEAD
        PyObject *cm_callable;
        PyObject *cm_dict;
    } classmethod;

![classmethod1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod1.png)

let's see what's under the hood

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

get a different result when access the same object in a different way, why?

when you trying to access the **fc1** in instance cc, the **descriptor protocol** will try several different paths to get the attribute in the following step
* call `__getattribute__` of the object C
* `C.__dict__["fc1"]` is a data descriptor?
    * yes, return `C.__dict__['fc1'].__get__(instance, Class)`
    * no, return `cc.__dict__['fc1']` if 'fc1' in `cc.__dict__` else
        * `C.__dict__['fc1'].__get__(instance, Class)` if hasattr(`C.__dict__['fc1']`, `__get__`) else `C.__dict__['fc1']`
* if not found in above steps, call `c.__getattr__("fc1")`

![object-attribute-lookup](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses/object-attribute-lookup.png)

for more detail, please refer to this blog [object-attribute-lookup](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses/#object-attribute-lookup) or [descr object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/descr.md)

![classmethod2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/classmethod2.png)

because **classmethod** implements `__get__` and `__set__`, it's a data descriptor, when you try to access attribute **cc.fc1**, you will actually call `fc1.__get__`, and caller will get whatever it returns

we can see the `__get__` function of classmethod object(defined as `cm_descr_get` in C)

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

when you access fc1 by **cc.fc1**, the **descriptor protocol** will call the function above, which returns whatever in the **cm_callable**, wrapped by **PyMethod_New()** function, which makes the return object a new bounded-PyMethodObject

##### staticmethod

the **@staticmethod** changes type of **c1.fs** to [function](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/func.md)

    >>> type(c1.fs)
    <class 'function'>

![staticmethod](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/class/staticmethod.png)

    typedef struct {
        PyObject_HEAD
        PyObject *sm_callable;
        PyObject *sm_dict;
    } staticmethod;

this is the layout of **staticmethod** object

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

we can see the `__get__` function of staticmethod object

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

so, when you access fs1 by **cc.fs1**, the **descriptor protocol** happens again, `C.__dict__["fs1"]__get__(instance, Class)` returns the **lambda** function