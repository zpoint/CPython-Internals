#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <string>
#include <future>
#include <vector>
#include <numpy/arrayobject.h>

PyObject* failure(PyObject *type, const char *message) {
    PyErr_SetString(type, message);
    return NULL;
}

int load_thread_num() {
    int ret = -1;
    char* num;
    num = getenv("CONCURRENCY_NUM");
    if (num != nullptr) {
        ret = std::stoi(num);
    }
    // printf("load_thread_num return: %d\n", ret);
    return ret > 1 ? ret : 1;
}

void parallel_run(PyArrayObject *origin_one_dim, PyArrayObject *origin_two_dim, int start, int end, double val) {
    npy_intp size_one_dim = PyArray_DIM(origin_one_dim, 1);
    for (int curr_start = 0; curr_start < end; ++ curr_start) {
        double* d_one_dim = (double*)PyArray_GETPTR2(origin_one_dim, curr_start, 0);
        double* d_two_dim_0 = (double*)PyArray_GETPTR2(origin_two_dim, curr_start, 0);
        double* d_two_dim_1 = (double*)PyArray_GETPTR2(origin_two_dim, curr_start, 1);
        double* d_two_dim_2 = (double*)PyArray_GETPTR2(origin_two_dim, curr_start, 2);
        for (npy_intp i = 0; i < size_one_dim; ++i) {
            d_one_dim[i] = (d_two_dim_0[i] + d_two_dim_1[i] + d_two_dim_2[i]) * val;
        }
    }
}

static PyObject* example(PyObject *self, PyObject *args) {
    /* def example(two_dim: np.array, one_dim: np.array, val: np.float) -> None */
    PyArrayObject *two_dim, *one_dim;
    double val;
    if (!PyArg_ParseTuple(args, "O!O!d", &PyArray_Type, &two_dim, &PyArray_Type, &one_dim, &val))
        return failure(PyExc_KeyError, "expected one three dimension array, one two dimension array and one double value");

    if (PyArray_DESCR(two_dim)->type_num != NPY_FLOAT64)
        return failure(PyExc_TypeError, "Type np.float expected for two_dim array.");

    if (PyArray_DESCR(one_dim)->type_num != NPY_FLOAT64)
        return failure(PyExc_TypeError, "Type np.float expected for one_dim array.");

    if (PyArray_NDIM(two_dim) != 3 || PyArray_DIM(two_dim, 1) != 3)
    return failure(PyExc_TypeError, "two_dim dimension error.");

    if (PyArray_NDIM(one_dim) != 2)
    return failure(PyExc_TypeError, "one_dim dimension error.");

    npy_intp dim1_one = PyArray_DIM(one_dim, 0);

    int right, steps;
    int thread_num = load_thread_num();
    if (thread_num > dim1_one)
        thread_num = dim1_one;
    std::vector<std::future<void>> future_vector;
    future_vector.reserve(thread_num);
    steps = dim1_one / thread_num;
    for (int i = 0; i < dim1_one; ) {
        // [i, i+steps)
        right = i + steps;
        if (right > dim1_one) {
            right = dim1_one;
        }
        future_vector.push_back(std::async(std::launch::async, parallel_run, one_dim, two_dim, i, right, val));
        i = right;
    }
    for (auto &future : future_vector) {
        future.get();
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
