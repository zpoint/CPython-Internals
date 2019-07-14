# thread

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [start_new_thread](#start_new_thread)
* [allocate_lock](#allocate_lock)
* [exit_thread](#exit_thread)
* [stack_size](#thread_stack_size)

# related file

* cpython/Modules/_threadmodule.c
* cpython/Python/thread.c
* cpython/Python/thread_nt.h
* cpython/Python/thread_pthread.h
* cpython/Python/pystate.c
* cpython/Include/cpython/pystate.h

# memory layout

a bootstate structure stores every informations a new python thread needed

![bootstate](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/bootstate.png)

# start_new_thread

	/* cpython/Modules/_threadmodule.c */
    static PyObject *
    thread_PyThread_start_new_thread(PyObject *self, PyObject *fargs)
    {
        PyObject *func, *args, *keyw = NULL;
        struct bootstate *boot;
        unsigned long ident;

        if (!PyArg_UnpackTuple(fargs, "start_new_thread", 2, 3,
                               &func, &args, &keyw))
            return NULL;
        if (!PyCallable_Check(func)) {
            PyErr_SetString(PyExc_TypeError,
                            "first arg must be callable");
            return NULL;
        }
        if (!PyTuple_Check(args)) {
            PyErr_SetString(PyExc_TypeError,
                            "2nd arg must be a tuple");
            return NULL;
        }
        if (keyw != NULL && !PyDict_Check(keyw)) {
            PyErr_SetString(PyExc_TypeError,
                            "optional 3rd arg must be a dictionary");
            return NULL;
        }
        boot = PyMem_NEW(struct bootstate, 1);
        if (boot == NULL)
            return PyErr_NoMemory();
        boot->interp = _PyInterpreterState_Get();
        boot->func = func;
        boot->args = args;
        boot->keyw = keyw;
        boot->tstate = _PyThreadState_Prealloc(boot->interp);
        if (boot->tstate == NULL) {
            PyMem_DEL(boot);
            return PyErr_NoMemory();
        }
        Py_INCREF(func);
        Py_INCREF(args);
        Py_XINCREF(keyw);
        PyEval_InitThreads(); /* Start the interpreter's thread-awareness */
        ident = PyThread_start_new_thread(t_bootstrap, (void*) boot);
        if (ident == PYTHREAD_INVALID_THREAD_ID) {
            PyErr_SetString(ThreadError, "can't start new thread");
            Py_DECREF(func);
            Py_DECREF(args);
            Py_XDECREF(keyw);
            PyThreadState_Clear(boot->tstate);
            PyMem_DEL(boot);
            return NULL;
        }
        return PyLong_FromUnsignedLong(ident);
    }

