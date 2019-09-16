# descr

Thanks @Hanaasagi for pointing out the errors [#19](https://github.com/zpoint/CPython-Internals/issues/19) of this article

# contents

* [related file](#related-file)
* [how does attribute access work in python?](#how-does-attribute-access-work-in-python)
	* [built-in types](#built-in-types)
        * [instance attribute access](#instance-attribute-access)
        * [class attribute access](#class-attribute-access)
    * [self defined types](#built-in-types)
    	* [self defined types instance attribute access](#self-defined-types-instance-attribute-access)
    	* [self defined types class attribute access](#self-defined-types-class-attribute-access)
* [method_descriptor](#method_descriptor)
    * [memory layout](#memory-layout)
* [how to change the behavior of attribute access?](#how-to-change-the-behavior-of-attribute-access)
* [read more](#read-more)

# related file

* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h
* cpython/Objects/object.c
* cpython/Include/object.h
* cpython/Objects/typeobject.c
* cpython/Include/cpython/object.h

# how does attribute access work in python

## built in types

let's see an example first before we look into how descriptor object implements

    print(type(str.center)) # <class 'method_descriptor'>
    print(type("str".center)) # <class 'builtin_function_or_method'>

what is type **method_descriptor** ? why will `str.center` returns a **method_descriptor** object, but `"str".center` returns a **builtin_function_or_method** ? how does attribute access work in python ?

### instance attribute access

this is the defination of `inspect.ismethoddescriptor` and `inspect.isdatadescriptor`

    def ismethoddescriptor(object):
        if isclass(object) or ismethod(object) or isfunction(object):
            # mutual exclusion
            return False
        tp = type(object)
        return hasattr(tp, "__get__") and not hasattr(tp, "__set__")

    def isdatadescriptor(object):
        """Return true if the object is a data descriptor.

        Data descriptors have both a __get__ and a __set__ attribute.  Examples are
        properties (defined in Python) and getsets and members (defined in C).
        Typically, data descriptors will also have __name__ and __doc__ attributes
        (properties, getsets, and members have both of these attributes), but this
        is not guaranteed."""
        if isclass(object) or ismethod(object) or isfunction(object):
            # mutual exclusion
            return False
        tp = type(object)
        return hasattr(tp, "__set__") and hasattr(tp, "__get__")

according to the comments, we know that **data descriptor** has both `__set__` and `__get__` attribute defined while **method descriptor** has only `__get__` defined

if we **dis** the `print(type(str.center))`

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

we can see that the core **opcode** is `LOAD_ATTR`, follow the `LOAD_ATTR` to the `Python/ceval.c`, we can find the definition

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
    // PyObject_GetAttr is defined in Objects/object.c
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
        /* first call the tp_getattro function in C level */
        if (tp->tp_getattro != NULL)
            return (*tp->tp_getattro)(v, name);
        /* if there's not a tp_getattro C function, try to call the tp_getattr function */
        if (tp->tp_getattr != NULL) {
            const char *name_str = PyUnicode_AsUTF8(name);
            if (name_str == NULL)
                return NULL;
            return (*tp->tp_getattr)(v, (char *)name_str);
        }
        /* If there's not a tp_getattr C function either, raise an exception */
        PyErr_Format(PyExc_AttributeError,
                     "'%.50s' object has no attribute '%U'",
                     tp->tp_name, name);
        return NULL;
    }

[**tp_getattro**](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_getattro) is

> an optional pointer to the get-attribute function.

it accepts two parameters, `PyObject *o, PyObject *attr_name`

[**tp_getattr**](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_getattr) is

> an optional pointer to the get-attribute-string function.
> This field is deprecated. When it is defined, it should point to a function that acts the same as the tp_getattro function, but taking a C string instead of a Python string object to give the attribute name.

it accepts two parameters, `PyObject *o, char *attr_name`

The only difference between them is the second parameter, which is of type `PyObject *` in **tp_getattro** and type `char *` in **tp_getattr**

From the above `PyObject_GetAttr` function, we can learn that `tp_getattro` have a higher priority than `tp_getattr`

we can see how type **str** is defined in `Objects/unicodeobject.c`

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

it's using the widely used c function `PyObject_GenericGetAttr` in cpython as it's `tp_getattro`, which is defined in `Objects/object.c`

    PyObject *
    PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
    {
        return _PyObject_GenericGetAttrWithDict(obj, name, NULL, 0);
    }

    PyObject *
    _PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                     PyObject *dict, int suppress)
    {
        /* Make sure the logic of _PyObject_GetMethod is in sync with
           this method.

           When suppress=1, this function suppress AttributeError.
        */

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
        /* look for a name through the MRO */
        descr = _PyType_Lookup(tp, name);

        f = NULL;
        if (descr != NULL) {
            /* the name in MRO can be found */
            Py_INCREF(descr);
            /* get the tp_descr_get field of descr object, which is defined as __get__ in python level */
            f = descr->ob_type->tp_descr_get;
            /* check if the object is data descriptor, PyDescr_IsData is defined as #define PyDescr_IsData(d) (Py_TYPE(d)->tp_descr_set != NULL) */
            if (f != NULL && PyDescr_IsData(descr)) {
                /* check success, means the object define both __get__ and __set__ */
                /* if it's data descriptor, try to call the __get__ function and stores the result in variable res */
                res = f(descr, obj, (PyObject *)obj->ob_type);
                if (res == NULL && suppress &&
                        PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    PyErr_Clear();
                }
                /* finish */
                goto done;
            }
        }
        /* down here, the descr is NULL(means not able to find the name through the MRO) or descr is not NULL, but without __set__ defined */
        if (dict == NULL) {
            /* find the __dict__ variables inside the object */
            /* Inline _PyObject_GetDictPtr */
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
        /* if the __dict__ is not NULL, try to find whether the name is stored inside the __dict__ */
            Py_INCREF(dict);
            res = PyDict_GetItem(dict, name);
            if (res != NULL) {
                Py_INCREF(res);
                Py_DECREF(dict);
                goto done;
            }
            Py_DECREF(dict);
        }
        /* down here, either __dict__ is NULL or name is not stored inside the __dict__ or __set__ not defined in descr */
        if (f != NULL) {
            /* __get__ is defined, but __set__ is not defined */
            /* try to call the __get__ function */
            res = f(descr, obj, (PyObject *)Py_TYPE(obj));
            if (res == NULL && suppress &&
                    PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
            }
            goto done;
        }
        /* down here, the descr doesn't define __get__ method and can't find name inside __dict__*/
        if (descr != NULL) {
            /* if name is found in MRO, return the found object */
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

we can draw the process according to the code above

![_str__attribute_access](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/_str__attribute_access.png)

until now, I made a mistake, the `tp_getattro` in `PyUnicode_Type` will not be called in `str.center`, otherwise, it will be called in `"str".center`, you can tell the differences in the following codes

    >>> type("str")
    <class 'str'>
    >>> "str".center # it calls the tp_getattro in PyUnicode_Type
    <built-in method center of str object at 0x10360f500>
    >>> type("str".center)
    <class 'builtin_function_or_method'>

    >>> type(str)
    <class 'type'>
    >>> str.center # it calls the tp_getattro in a type named PyType_Type
    <method 'center' of 'str' objects>
    >>> type(str.center)
    <class 'method_descriptor'>

so, the procedure above describes the attribute access of `"str".center`

### class attribute access

let's find the definition of `<class 'type'>` and how exactly `str.center` works (mostly same as `"str".center`)

for the type `<class 'type'>`, the `LOAD_ATTR` calls the `type_getattro`

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
            logic mostly same as _PyObject_GenericGetAttrWithDict,
            for thoes who are interested, read Objects/typeobject.c directly
        */
    }

![str_attribute_access](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/str_attribute_access.png)

from the above pictures and codes, we can know that even if the `tp_getattro` function of `str` and `"str"` are different, they both calls the same `tp_descr_get` (alias of `__get__`) defined in a type named `method_descriptor`

    /* in cpython/Objects/descrobject.c */
    static PyObject *
    method_get(PyMethodDescrObject *descr, PyObject *obj, PyObject *type)
    {
        PyObject *res;
        /* descr_check checks whether the descriptor was found on the target object itself (or a base) */
        if (descr_check((PyDescrObject *)descr, obj, &res))
            /* str.center goes into this branch, returns the type of PyMethodDescrObject */
            return res;
        /* while "str".center goes into this branch, returns a type of PyCFunction */
        return PyCFunction_NewEx(descr->d_method, obj, NULL);
    }

now, we have the answers of
* why will `str.center` returns a **method_descriptor** object, but `"str".center` returns a **builtin_function_or_method** ?
* how does attribute access work in python ?

## self defined types

Assume you've read the following two parts

* [how does attributes initialize in the creation of class/instance](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/slot/slot.md#without-slots)
* [creation of class/instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/type.md#creation-of-class)

### self defined types instance attribute access

From the above analyzation, we can laern that the key point is `tp_getattro` defined in `type(instance)`, `tp_getattro` function of those built-in types is pre defined in C, while user defined type is created on the fly, some function will be attached to the newly created type in the `tp_getattro` field

    class A(object):
        def __getattr__(self, item):
            pass


    class B(object):
        def __getattribute__(self, item):
            pass


    class C(object):
        pass

What is in the `tp_getattro` field of `class A`, `class B` and `class C` ?

The following codes are snippests of the creation of a type

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
            /* Put the proper slots in place */
            fixup_slot_dispatchers(type);
            /* ... */
    }

    /* Store the proper functions in the slot dispatches at class (type)
       definition time, based upon which operations the class overrides in its
       dict. */
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

We can learn that for every pre defined slot attribute in the `slotdefs`, `update_one_slot` will install the proper function/object to the newly created type

`__getattribute__` and `__getattr__` have same offset in every newly created type(you can learn from `slotdefs` structure)

For `class A`, when installing `__getattribute__`, `offset` is `144`, the `descr` is `<slot wrapper '__getattribute__' of 'object' objects>`, `PyType_IsSubtype(type, PyDescr_TYPE(d))` is True, so `specific` will be `d->d_wrapped`, which is `PyObject_GenericGetAttr`. In the next while loop, when installing `__getattr__`, `offset` is also `144`, this time `descr` will be `<function A.__getattr__ at 0x1013260c0>`, the final `else` inside the while loop will set `generic` to `p->function`, which is `slot_tp_getattr_hook`, now the while loop terminate and offset `144` stores `slot_tp_getattr_hook`

For `class B`, when installing `__getattribute__`, `offset` is `144`, the `descr` is `<function B.__getattribute__ at 0x103ad6140>`,  the final `else` inside the while loop will set `generic` to `p->function`, which is `slot_tp_getattr_hook`, In the next while loop, when installing `__getattr__`, `offset` is also `144`, this time `descr` is a `null` pointer, so the `continue` statement will terminate the while loop, offset `144` stores `slot_tp_getattr_hook`

For `class C`, when installing `__getattribute__`, `offset` is `144`, the `descr` is `<slot wrapper '__getattribute__' of 'object' objects>`, `PyType_IsSubtype(type, PyDescr_TYPE(d))` is True, so `specific` will be `d->d_wrapped`, which is `PyObject_GenericGetAttr`, In the next while loop, when installing `__getattr__`, `offset` is also `144`, this time `descr` is a `null` pointer, so the `continue` statement will terminate the while loop, offset `144` stores `PyObject_GenericGetAttr`

`slot_tp_getattr_hook` is defined as

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

For `slot_tp_getattr_hook`

* If there's no `___getattr__` method, `slot_tp_getattr_hook` will only call `___getattribute__` directly

* If there defines `__getattr__`, `slot_tp_getattr_hook` will call `___getattribute__`, if there's no result and occurs `PyExc_AttributeError`, try to call `__getattr__`

If you override any of `__getattribute__` or `__getattr__` , `tp_getattro` of the newly created class will be `slot_tp_getattr_hook`

If none of `__getattribute__` or `__getattr__` are override, `tp_getattro` of the newly created class will be `PyObject_GenericGetAttr`(same as built-in types)

![user_defined_descr](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/user_defined_descr.png)

### self defined types class attribute access

Because `type(newly_created_class)` will always return `<class 'type'>`, and the `tp_getattro` of `<class 'type'>` is pre defined in C and not able to be customized, the behaviour of attribute accessing is same as [class attribute access](#class-attribute-access)

# method_descriptor

let's find out the answer of
* what is type **method_descriptor** ?

it's defined in `Include/descrobject.h`

## memory layout

![PyMethodDescrObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/PyMethodDescrObject.png)

we can see that when you try to access `"str".center` and `str.center`, they both shares the same **PyMethodDescrObject**, which is a wrapper of the **PyMethodDef** object

for those who are interested in **PyMethodDef**, please refer to [method](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method.md)

    >>> str.center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370
    >>> "str".center
    descr->d_common->d_type: 0x10fdefc10, descr->d_common->d_type.tp_name: str, repr(d_name): 'center', repr(d_qualname): NULL, PyMethodDef: 0x10fdf0370

there exists various descriptor type

**PyMemberDescrObject**: wrapper of **PyMemberDef**

**PyGetSetDescrObject**: wrapper of **PyGetSetDef**

# how to change the behavior of attribute access

we know that when you try to access the attribute of an object, the python virtual machine will

1. execute the opcode `LOAD_ATTR`
2. `LOAD_ATTR` will try to call `tp_getattro` function of the object, if success go to 4
3. raise an exception
4. return what's returned

the default `tp_getattro` will be installed in the creation process of the newly created type, different behaviour depends on what methods are override by user, generally, the default `tp_getattro` is `PyObject_GenericGetAttr` which implements the **descriptor protocol**(we learned above from the source code)

when we define a python object, if we need to change the behavior of attribute access

we are not able to change the behavior of opcode `LOAD_ATTR`, it's written in C

instead, we can provide our own `__getattribute__` and `__getattr__` to change the function installed in the `tp_getattro` slot of the newly created type

notice, provide your own `__getattribute__` may violate the **descriptor protocol**, I will not recommend you to do that(usually we only need to define our own `__getattr__`)

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

# read more
* [descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)
* [The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)