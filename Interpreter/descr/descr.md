# descr

### contents

* [related file](#related-file)
* [how does attribute access work in python?](#how-does-attribute-access-work-in-python?)


#### related file

* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h
* cpython/Objects/object.c
* cpython/Include/object.h

#### how does attribute access work in python?

let's see an example first before we look into how descriptor object implements

	print(type(str.center)) # <class 'method_descriptor'>

what is type **method_descriptor** ? why will the str.center returns a **method_descriptor** object ? how does attribute access work in python ?

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

according to the comments, we know that type **method_descriptor** or type name ends with **_descriptor** are the objects that getsets and members defined in C

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

we can see that the core **opcode** is `LOAD_ATTR`, follow the `LOAD_ATTR` to the `Python/ceval.c`, we can find the defination

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
        /* first call the __getattribute__ method */
        if (tp->tp_getattro != NULL)
            return (*tp->tp_getattro)(v, name);
        /* if __getattribute__ fail, try to call the __getattr__ method */
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

**tp_getattro** is the `__getattribute__` method

**tp_getattr** is the `__getattr__` method

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

it's using the most widely used c function `PyObject_GenericGetAttr` as it's `tp_getattro` (alias of ` __getattribute__`), which is defined in `Objects/object.c`

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
        /* down here, the descr is NULL, means not able to find the name through the MRO  or descr with __get__ defined but __get__ not defined */
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

![attribute_access](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/descr/attribute_access.md)

until now, I made a mistake, the `tp_getattro` (alias of ` __getattribute__`) in `PyUnicode_Type` will not be called in `str.center`, otherwise it will be called in `"str".center`, you can tell the differences in the folloing codes

    >>> type("str")
    <class 'str'>
    >>> "str".center # it calls the tp_getattro in PyUnicode_Type
    <built-in method center of str object at 0x10360f500>
    >>> type("str".center)
	<class 'builtin_function_or_method'>

    >>> type(str)
    <class 'type'>
    >>> str.center # it may calls the tp_getattro in a type named PyType_Type
    <method 'center' of 'str' objects>
    >>> type(str.center)
	<class 'method_descriptor'>

let's find the defination of `<class 'type'>`

for more information please refer to [descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)

    from datetime import datetime
    dt = datetime.date
    print(type(dt))

