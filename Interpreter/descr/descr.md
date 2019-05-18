# descr

### contents

* [related file](#related-file)
* [how does attribute access work in python?](#how-does-attribute-access-work-in-python?)


#### related file
* cpython/Objects/descrobject.c
* cpython/Include/descrobject.h

#### how object attribute accessed in python?

we should see an example first before we look into how descriptor implements

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
        if (tp->tp_getattro != NULL)
            return (*tp->tp_getattro)(v, name);
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

for more information please refer to [descriptor protocol in python](https://docs.python.org/3/howto/descriptor.html)

    from datetime import datetime
    dt = datetime.date
    print(type(dt))

