#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

PyObject* failure(PyObject *type, const char *message) {
    PyErr_SetString(type, message);
    return NULL;
}

static PyObject* example(PyObject *self, PyObject *args) {
    /* def example(arr: typing.List[int]) -> int: */
    PyArrayObject *two_dim, *one_dim;
    double val;
    if (!PyArg_ParseTuple(args, "O!O!d", &PyArray_Type, &two_dim, &PyArray_Type, &one_dim, &val))
        return failure(PyExc_KeyError, "expected one two dimension array, one one dimension array and one double value");

    if (PyArray_DESCR(two_dim)->type_num != NPY_FLOAT64)
        return failure(PyExc_TypeError, "Type np.float expected for two_dim array.");

    if (PyArray_DESCR(one_dim)->type_num != NPY_FLOAT64)
        return failure(PyExc_TypeError, "Type np.float expected for one_dim array.");

    if (PyArray_NDIM(two_dim) != 2 || PyArray_DIM(two_dim, 0) != 3)
    return failure(PyExc_TypeError, "two_dim dimension error.");

    if (PyArray_NDIM(one_dim) != 1)
    return failure(PyExc_TypeError, "one_dim dimension error.");

    npy_intp size_one_dim = PyArray_SIZE(one_dim);
    double* d_one_dim = (double*)PyArray_GETPTR1(one_dim, 0);
    double* d_two_dim_0 = (double*)PyArray_GETPTR1(two_dim, 0);
    double* d_two_dim_1 = (double*)PyArray_GETPTR1(two_dim, 1);
    double* d_two_dim_2 = (double*)PyArray_GETPTR1(two_dim, 2);
    for (npy_intp i = 0; i < size_one_dim; ++i) {
        d_one_dim[i] = (d_two_dim_0[i] + d_two_dim_1[i] + d_two_dim_2[i]) * val;
    }
    return Py_None;

}

static PyMethodDef MyMethods[] = {
    {"example", example, METH_VARARGS, "example function"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef m_example = {
   PyModuleDef_HEAD_INIT,
   "example",
   "example module to provide numpy python api",
   -1,
   MyMethods
};

PyMODINIT_FUNC PyInit_m_example(void) {
    PyObject *m;

    m = PyModule_Create(&m_example);
    if (m == NULL)
        return NULL;
    import_array();
    return m;
}
