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

**thread.c** import **thread_nt.h** or **thread_pthread.h**, depending on the compiling machine's operating system

**thread_nt.h** and **thread_pthread.h** both define the same functionality API, delegate the thread creation or lock procedure to the related system call

![thread](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread.png)

# example

    from threading import Thread, currentThread, activeCount

    def my_func(a, b, c=4):
        print(a, b, c, currentThread().ident, activeCount())

    Thread(target=my_func, args=("hello world", "Who are you")).start()

module **threading** is a wrapper for the built-in **_thread** module, **threading** is written in pure python, you can read the source code directly

# start_new_thread

**PyEval_InitThreads** will create [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md) if it's not created before, thread scheduling strategy has changed after python3.2, there's no need to give another thread a chance to run periodly even if there's only one python intepreter thread

	/* cpython/Modules/_threadmodule.c */
    static PyObject *
    thread_PyThread_start_new_thread(PyObject *self, PyObject *fargs)
    {
        PyObject *func, *args, *keyw = NULL;
        struct bootstate *boot;
        unsigned long ident;

		/* ignore some detail
        unpack func, args, keyw form fargs, and check their type
        if you are using threading like the above example, these unpacked objects will look like
        func: <bound method Thread._bootstrap of <Thread(Thread-1, initial)>>
        args: ()
        keyw: NULL
        */
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
        /* PyEval_InitThreads will create gil if it's not created before */
        PyEval_InitThreads();
        /* delegate to the system call, in my platform, it's pthread_create */
        ident = PyThread_start_new_thread(t_bootstrap, (void*) boot);
        if (ident == PYTHREAD_INVALID_THREAD_ID) {
            /* handle error */
        }
        return PyLong_FromUnsignedLong(ident);
    }

allocate_lock

this is the normal lock object

	>>> r = _thread.allocate_lock()
	>>> type(r)
    _thread.lock
    >>> repr(r)
    '<unlocked _thread.lock object at 0x10abdf148>'
    >>> r.acquire_lock()
    True
    >>> repr(r)
    '<locked _thread.lock object at 0x10abdf148>'

![lock_object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/lock_object.png)

in the posix system, the defination and allocation of **lock_lock**

	/* cpython/Include/pythread.h */
	typedef void *PyThread_type_lock;

	/* cpython/Python/thread_pthread.h */
    PyThread_allocate_lock(void)
    {
        sem_t *lock;
        int status, error = 0;

        dprintf(("PyThread_allocate_lock called\n"));
        if (!initialized)
            PyThread_init_thread();

        lock = (sem_t *)PyMem_RawMalloc(sizeof(sem_t));

        if (lock) {
            status = sem_init(lock,0,1);
            CHECK_STATUS("sem_init");

            if (error) {
                PyMem_RawFree((void *)lock);
                lock = NULL;
            }
        }

        dprintf(("PyThread_allocate_lock() -> %p\n", lock));
        return (PyThread_type_lock)lock;
    }