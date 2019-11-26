/* the following code is only tested on Mac */
#define PY_SSIZE_T_CLEAN
/* check your system path, sometimes it's <Python.h> */
#include <Python/Python.h>

static PyObject* my_cpu_bound_task(PyObject *self, PyObject *args)
{
    /* this example is for illustration
       you need to do more check(overflow...) for production case */
    PyObject *py_x, *py_y, *global, *obj_i, *meaningless_dict;
    long x, y;
    static unsigned char r[1000];
    static PyObject* meaningless_py_str = NULL;

    if (meaningless_py_str == NULL)
    {
        // it should be check if allocate successfully and free if necessary, I don't do that here
        meaningless_py_str = PyString_FromString("meaningless_dict");
    }

    if (!PyArg_ParseTuple(args, "OO", &py_x, &py_y))
    {
        /* this step is not necessary, the purpose is to illustrate how to extract the PyObject from args */
        PyErr_SetString(PyExc_KeyError, "expected two int parameters, x and y");
        return NULL;
    }
    else if (!PyArg_ParseTuple(args, "ll", &x, &y))
    {
        PyErr_SetString(PyExc_KeyError, "expected two int parameters, x and y");
        return NULL;
    }

    global = PyEval_GetGlobals();
    if (global == NULL)
    {
        PyErr_SetString(PyExc_ValueError, "no frame is currently executing");
        return NULL;
    }

    meaningless_dict = PyDict_GetItem(global, meaningless_py_str);
    if (meaningless_dict == NULL || !PyDict_CheckExact(meaningless_dict))
    {
        PyErr_SetString(PyExc_KeyError, "unable to find meaningless_dict in global variables or meaningless_dict is not of type dict");
        return NULL;
    }

    memset(r, 0, 1000);
    for (Py_ssize_t index = 0; index < 1000; ++index)
    {
        obj_i = PyLong_FromLong(index);
        if (obj_i == NULL)
        {
            PyErr_SetString(PyExc_ValueError, "fail to create a py long object");
            return NULL;
        }
        if (PyDict_Contains(meaningless_dict, obj_i))
        {
            y += 1;
        }
        Py_XDECREF(obj_i);

        x += index;
        y = (x % 256) ^ (y ^ 256);
        r[index] = y;

    }
    return PyString_FromStringAndSize((const char *)r, 1000);
}

static PyMethodDef MyMethods[] = {
    {"my_cpu_bound_task", my_cpu_bound_task, METH_VARARGS, "compute my task in a faster way"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initmy_module(void)
{
    PyObject *m;

    m = Py_InitModule("my_module", MyMethods);
    if (m == NULL)
        return;
}
