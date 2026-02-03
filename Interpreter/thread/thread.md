# thread

# contents

* [related file](#related-file)
* [prerequisites](#prerequisites)
* [memory layout](#memory-layout)
* [start_new_thread](#start_new_thread)
* [allocate_lock](#allocate_lock)
* [allocate_rlock](#allocate_rlock)
* [exit_thread](#exit_thread)
* [stack_size](#thread_stack_size)
* [thread local](#thread-local)

# related file

* cpython/Modules/_threadmodule.c
* cpython/Python/thread.c
* cpython/Python/thread_nt.h
* cpython/Python/thread_pthread.h
* cpython/Python/pystate.c
* cpython/Include/cpython/pystate.h

# prerequisites

The following contents will show you how CPython implements thread-related functions. You will be able to get the answer to "Is POSIX semaphore or mutex used as a Python thread lock?"

If you are confused about what **POSIX thread** is or what **POSIX semaphore** is, you need to refer to Chapter 11 and Chapter 12 of [APUE](https://www.amazon.com/Advanced-Programming-UNIX-Environment-3rd/dp/0321637739) and [UNP vol 2](https://www.amazon.com/UNIX-Network-Programming-Interprocess-Communications/dp/0130810819)

If you are interested in how thread/interpreter is organized, please refer to [overview](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/pyobject/pyobject.md#intepreter-and-thread)

# memory layout

A bootstate structure stores all the information a new Python thread needs

![bootstate](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/bootstate.png)

**thread.c** import **thread_nt.h** or **thread_pthread.h**, depending on the compiling machine's operating system

**thread_nt.h** and **thread_pthread.h** both define the same functionality API, delegate the thread creation or lock procedure to the related system call

![thread](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread.png)

The following content will only show the POSIX part defined in **thread_pthread.h**. For other platforms, even if the API of the system call is different, the idea is the same

Notice, you're not encouraged to use module `_thread` directly. Use `threading` instead. The following code uses `_thread` for illustration

# example

```python3
from threading import Thread, currentThread, activeCount

def my_func(a, b, c=4):
    print(a, b, c, currentThread().ident, activeCount())

Thread(target=my_func, args=("hello world", "Who are you")).start()

```

module **threading** is a wrapper for the built-in **_thread** module, **threading** is written in pure python, you can read the source code directly

# start_new_thread

**PyEval_InitThreads** will create [GIL](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md) if it's not created before. Thread scheduling strategy has changed after Python 3.2. There's no need to give another thread a chance to run periodically when there's only one Python interpreter thread

```c
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

```

# allocate_lock

This is the normal lock object

```python3
>>> r = _thread.allocate_lock()
>>> type(r)
_thread.lock
>>> repr(r)
'<unlocked _thread.lock object at 0x10abdf148>'
>>> r.acquire_lock()
True
>>> repr(r)
'<locked _thread.lock object at 0x10abdf148>'

```

![lock_object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/lock_object.png)

In the POSIX system, the definition and allocation of **lock_lock**

```c
/* cpython/Include/pythread.h */
typedef void *PyThread_type_lock;

/* cpython/Python/thread_pthread.h */
#ifdef USE_SEMAPHORES
...
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
...
#else /* USE_SEMAPHORES */
...
PyThread_type_lock
PyThread_allocate_lock(void)
{
	/* implement with pthread mutex */
}

```

From the above code we can see that CPython prefers to implement the field in **lock_lock** as [POSIX semaphores](http://www.csc.villanova.edu/~mdamian/threads/posixsem.html) over [POSIX mutex](http://www.skrenta.com/rt/man/pthread_mutex_init.3.html). The examples below use semaphores for illustration

```python3
>>> r.acquire_lock()

```

If you try to acquire the lock, several POSIX system calls will be invoked according to the timeout value

```python3
/* cpython/Python/thread_pthread.h */
while (1) {
    if (microseconds > 0) {
        status = fix_status(sem_timedwait(thelock, &ts));
    }
    else if (microseconds == 0) {
        status = fix_status(sem_trywait(thelock));
    }
    else {
        status = fix_status(sem_wait(thelock));
    }

    /* Retry if interrupted by a signal, unless the caller wants to be
       notified.  */
    if (intr_flag || status != EINTR) {
        break;
    }

    if (microseconds > 0) {
        /* wait interrupted by a signal (EINTR): recompute the timeout */
    }
}

```

If you acquire the lock successfully, the locked field will become 1

![lock_object_locked](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/lock_object_locked.png)

# allocate_rlock

**RLock** is the abbreviation of recursive lock. As [Wikipedia](https://en.wikipedia.org/wiki/Reentrant_mutex) says

> In computer science, the reentrant mutex (recursive mutex, recursive lock) is a particular type of mutual exclusion (mutex) device that may be locked multiple times by the same process/thread, without causing a deadlock.

![rlock_object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object.png)

```python3

>>> r = _thread.RLock()

```

The procedure of allocation of **lock_lock** in the RLock object is the same as the above example

Before acquiring the lock, **rlock_owner** and **rlock_count** are both set to 0

```python3
>>> r.acquire()

```

![rlock_object_acquire](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object_acquire.png)

```python3
>>> r.acquire()

```

![rlock_object_acquire2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object_acquire2.png)

We found that **rlock_owner** is the current thread's ident, and **rlock_count** is a counter of how many times the lock is currently acquired

The acquire procedure is very clear

```c
/* cpython/Modules/_threadmodule.c */
static PyObject *
rlock_acquire(rlockobject *self, PyObject *args, PyObject *kwds)
{
    _PyTime_t timeout;
    unsigned long tid;
    PyLockStatus r = PY_LOCK_ACQUIRED;

    if (lock_acquire_parse_args(args, kwds, &timeout) < 0)
        return NULL;

    tid = PyThread_get_thread_ident();
    if (self->rlock_count > 0 && tid == self->rlock_owner) {
    	/* acquire before, just increase the rlock_count */
        unsigned long count = self->rlock_count + 1;
        if (count <= self->rlock_count) {
            PyErr_SetString(PyExc_OverflowError,
                            "Internal lock count overflowed");
            return NULL;
        }
        self->rlock_count = count;
        Py_RETURN_TRUE;
    }
	/* delegate the acquire to the posix system call */
    r = acquire_timed(self->rlock_lock, timeout);
    if (r == PY_LOCK_ACQUIRED) {
    	/* if never acquired before, success in acquiring the lock */
        assert(self->rlock_count == 0);
        self->rlock_owner = tid;
        self->rlock_count = 1;
    }
    else if (r == PY_LOCK_INTR) {
    	/* the lock is acquired by another thread, fail in acquiring the lock */
        return NULL;
    }
    return PyBool_FromLong(r == PY_LOCK_ACQUIRED);
}


```

The release procedure

```c
/* cpython/Modules/_threadmodule.c */
static PyObject *
rlock_release(rlockobject *self, PyObject *Py_UNUSED(ignored))
{
    unsigned long tid = PyThread_get_thread_ident();
	/* check if the rlock_count reaches 0 or release by other thread */
    if (self->rlock_count == 0 || self->rlock_owner != tid) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot release un-acquired lock");
        return NULL;
    }
    /* decrement the rlock_count
    reset rlock_owner to give other thread a chance to acquire the lock if rlock_count reaches 0 */
    if (--self->rlock_count == 0) {
        self->rlock_owner = 0;
        PyThread_release_lock(self->rlock_lock);
    }
    Py_RETURN_NONE;
}

```

# exit_thread

```c
/* cpython/Modules/_threadmodule.c */
static PyObject *
thread_PyThread_exit_thread(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    /* just raise SystemExit exception */
    PyErr_SetNone(PyExc_SystemExit);
    return NULL;
}

```

# stack_size

You can set the stack size to 100KB by calling

```python3
>>> _thread.stack_size(102400) # threading.stack_size(102400)

```

which will delegate to [`pthread_attr_setstacksize`](http://man7.org/linux/man-pages/man3/pthread_attr_setstacksize.3.html)

```c
_pythread_pthread_set_stacksize(size_t size)
{
#if defined(THREAD_STACK_SIZE)
    pthread_attr_t attrs;
    size_t tss_min;
    int rc = 0;
#endif

    /* set to default */
    if (size == 0) {
        _PyInterpreterState_GET_UNSAFE()->pythread_stacksize = 0;
        return 0;
    }

#if defined(THREAD_STACK_SIZE)
#if defined(PTHREAD_STACK_MIN)
    tss_min = PTHREAD_STACK_MIN > THREAD_STACK_MIN ? PTHREAD_STACK_MIN
                                                   : THREAD_STACK_MIN;
#else
    tss_min = THREAD_STACK_MIN;
#endif
    if (size >= tss_min) {
        /* validate stack size by setting thread attribute */
        if (pthread_attr_init(&attrs) == 0) {
            rc = pthread_attr_setstacksize(&attrs, size);
            pthread_attr_destroy(&attrs);
            if (rc == 0) {
                _PyInterpreterState_GET_UNSAFE()->pythread_stacksize = size;
                return 0;
            }
        }
    }
    return -1;
#else
    return -2;
#endif
}

```

# thread local

The implementation of thread local storage is a per-thread [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict.md) object stored inside the **PyThreadState** structure
