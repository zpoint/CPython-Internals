/* check your system path, sometimes it's <Python/Python.h> */
/* only for illustration purpose, not good code for production */
#include <Python.h>
#include <vector>
#include <algorithm>

static PyObject* example(PyObject *self, PyObject *args) {
    /* def example(arr: typing.List[int]) -> int: */
    PyObject *py_arr;
    std::vector<int> i_vec;
    long res;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &py_arr)) {
        PyErr_SetString(PyExc_ValueError, "expected one list parameter");
        return nullptr;
    }
    Py_ssize_t list_len = PyList_GET_SIZE(py_arr);
    i_vec.reserve(list_len);
    for (Py_ssize_t i = 0; i < list_len; ++i) {
        PyObject *py_int = PyList_GET_ITEM(py_arr, i);
        res = PyLong_AsLong(py_int);
        if (res == -1 && PyErr_Occurred()) {
            return nullptr;
        }
        i_vec.push_back(res);
    }
    std::sort(i_vec.begin(), i_vec.end());
    return PyLong_FromLong(i_vec[0]);
}

static PyMethodDef MyMethods[] = {
    {"example", example, METH_VARARGS, "example function"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef m_example = {
   PyModuleDef_HEAD_INIT,
   "example",
   "example module to provide python api",
   -1,
   MyMethods
};

PyMODINIT_FUNC PyInit_m_example(void) {
    PyObject *m;

    m = PyModule_Create(&m_example);
    if (m == NULL)
        return NULL;
    return m;
}