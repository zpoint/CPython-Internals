# GIL

# Contents

* [related file](#related-file)
* [introduction](#introduction)
	* [thread scheduling before python3.2](#thread-scheduling-before-python32)
	* [thread scheduling after python3.2](#thread-scheduling-after-python32)
	* [memory layout](#memory-layout)
* [fields](#fields)
	* [interval](#interval)
	* [last_holder](#last_holder)
	* [locked](#locked)
	* [switch_number](#switch_number)
	* [mutex](#mutex)
	* [cond](#cond)
	* [switch_cond and switch_mutex](#switch_cond-and-switch_mutex)
* [when will the gil be released](#when-will-the-gil-be-released)

# Related files

* cpython/Python/ceval.c
* cpython/Python/ceval_gil.h
* cpython/Include/internal/pycore_gil.h

# Introduction

This is the definition of the [**Global Interpreter Lock**](https://wiki.python.org/moin/GlobalInterpreterLock).

> In CPython, the global interpreter lock, or GIL, is a mutex that protects access to Python objects, preventing multiple threads from executing Python bytecodes at once. This lock is necessary mainly because CPython's memory management is not thread-safe. (However, since the GIL exists, other features have grown to depend on the guarantees that it enforces.)

## Thread scheduling before Python 3.2

Basically, the **tick** is a counter for how many opcodes the current thread has executed continuously without releasing the **GIL**.

If the current thread is running a CPU-bound task, it will release the **gil** and offer an opportunity for another thread to run for every 100 **ticks**.

If the current thread is running an IO-bound task, the **GIL** will be released manually if you call `sleep/recv/send(...etc)` even without count to 100 **ticks**.

You can call `sys.setcheckinterval()` to set other **tick** count value instead of 100.

![old_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/old_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

Because the **tick** is not time-based, some threads might run far longer than others.

On a multi-core machine, if two threads are both running CPU-bound tasks, the OS might schedule them to run on different cores. There might be a situation where one thread holding the **GIL** is executing its task in its **100 ticks cycle** on one core, while the thread on the other core wakes up periodically trying to acquire the **GIL** but fails, spinning the CPU.

The job (thread) scheduling mechanism is fully controlled by the operating system. The thread handling an IO-bound task has to wait for another thread to release the **GIL**, and that other thread might re-acquire the **GIL** after releasing it, which makes the current IO-bound task thread wait even longer (actually, the thread that causes the OS's context-switch by itself will have higher priority than those threads forced by the OS; programmers can utilize this feature by putting the IO-bound thread to sleep as soon as possible).

![gil_battle](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_battle.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

## Thread scheduling after Python 3.2

Due to some performance issues on multi-core machines, the implementation of the **GIL** has changed a lot since Python 3.2.

If there's only one thread, it can run forever without checking and releasing the **GIL**.

If there is more than one thread, the thread currently blocked by the **GIL** will wait for a timeout period and set the **gil_drop_request** to 1, then continue waiting. The thread currently holding the **GIL** will release the **GIL** and wait for the same timeout period if the **gil_drop_request** is set to 1. The thread currently blocking will be signaled and is able to acquire the **GIL**.

![new_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

The thread that sets **gil_drop_request** to 1 might not be the thread that acquires the **GIL**.

If the current thread is waiting for the interval and the owner of the **GIL** changes during the waiting **interval**, after waking up, the current thread needs to wait, set **gil_drop_request** to 1, and wait again.

![new_gil2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil2.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

For those who are interested in further details, please refer to [Understanding the Python GIL(article)](http://www.dabeaz.com/GIL/).

## Memory layout

![git_layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_layout.png)

# Fields

The Python interpreter is a program written in C. Every executable program written in C has a `main` function.

Those `main`-related functions are defined in `cpython/Modules/main.c`. You will find that the `main`-related function does some initialization for the interpreter status before executing the `main loop`. The `_gil_runtime_state` will be created and initialized during this initialization.

```python3
./python.exe

```

![init](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/init.png)

## interval

```python3
>>> import sys
>>> sys.getswitchinterval()
0.005

```

**interval** is the suspend timeout before setting the `gil_drop_request` in microseconds, 5000 microseconds is 0.005 seconds.

It's stored as microseconds in C and represented as seconds in Python.

## last_holder

**last_holder** stores the C address of the last `PyThreadState` holding the **GIL**, this helps us know whether anyone else was scheduled after we dropped the **GIL**.

## locked

**locked** is a field of type **_Py_atomic_int**, -1 indicates uninitialized, 0 means no one is currently holding the **GIL**, 1 means someone is holding it. This is atomic because it can be read without any lock taken in `ceval.c`.

```c
/* cpython/Python/ceval_gil.h */
static void take_gil(PyThreadState *tstate)
{
    /* omit */
    /* We now hold the GIL */
    _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 1);
    _Py_ANNOTATE_RWLOCK_ACQUIRED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);
    if (tstate != (PyThreadState*)_Py_atomic_load_relaxed(
                    &_PyRuntime.ceval.gil.last_holder))
    {
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                 (uintptr_t)tstate);
        ++_PyRuntime.ceval.gil.switch_number;
    }
    /* omit */
}

static void drop_gil(PyThreadState *tstate)
{
    /* omit */
    if (tstate != NULL) {
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                 (uintptr_t)tstate);
    }
    MUTEX_LOCK(_PyRuntime.ceval.gil.mutex);
    _Py_ANNOTATE_RWLOCK_RELEASED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);
    _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 0);
    /* omit */
}

```

## switch_number

**switch_number** is a counter for the number of **GIL** switches since the beginning.

It's used in function `take_gil`.

```c
static void take_gil(PyThreadState *tstate)
{
    /* omit */
    while (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked)) {
    	/* as long as the gil is locked */
        int timed_out = 0;
        unsigned long saved_switchnum;

        saved_switchnum = _PyRuntime.ceval.gil.switch_number;
        /* release gil.mutex, wait for INTERVAL microseconds(default 5000)
        or gil.cond is signaled during the INTERVAL
        */
        COND_TIMED_WAIT(_PyRuntime.ceval.gil.cond, _PyRuntime.ceval.gil.mutex,
                        INTERVAL, timed_out);
        /* currently holding gil.mutex */
        if (timed_out &&
            _Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked) &&
            _PyRuntime.ceval.gil.switch_number == saved_switchnum) {
            /* If we timed out and no switch occurred in the meantime, it is time
           	to ask the GIL-holding thread to drop it.
            set gil_drop_request to 1 */
            SET_GIL_DROP_REQUEST();
        }
        /* go on to the while loop to check if the gil is locked */
    }
    /* omit */
}

```

## mutex

**mutex** is a mutex used for protecting `locked`, `last_holder`, `switch_number`, and other variables in `_gil_runtime_state`.

## cond

**cond** is a condition variable, combined with **mutex**, used for signaling  the release of the **GIL**.

## switch_cond and switch_mutex

**switch_cond** is another condition variable, combined with **switch_mutex** can be used for making sure that the thread acquiring the **GIL** is not the thread that released the **GIL**, avoiding a waste of the time slice.

It can be turned off without the definition of `FORCE_SWITCHING`.

```c
static void drop_gil(PyThreadState *tstate)
{
/* omit */
#ifdef FORCE_SWITCHING
    if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil_drop_request) &&
        tstate != NULL)
    {
    	/* if the gil_drop_request is set and tstate is not null */
        /* lock the mutex switch_mutex */
        MUTEX_LOCK(_PyRuntime.ceval.gil.switch_mutex);
        if (((PyThreadState*)_Py_atomic_load_relaxed(
                    &_PyRuntime.ceval.gil.last_holder)
            ) == tstate)
        {
        /* if the last_holder is the current thread, release the switch_mutex,
        wait until there's a signal for switch_cond */
        RESET_GIL_DROP_REQUEST();
            /* NOTE: if COND_WAIT does not atomically start waiting when
               releasing the mutex, another thread can run through, take
               the GIL and drop it again, and reset the condition
               before we even had a chance to wait for it. */
            COND_WAIT(_PyRuntime.ceval.gil.switch_cond,
                      _PyRuntime.ceval.gil.switch_mutex);
    }
        MUTEX_UNLOCK(_PyRuntime.ceval.gil.switch_mutex);
    }
#endif
}

```

# When will the GIL be released

The `main_loop` in `cpython/Python/ceval.c` is a big `for loop`, and a big `switch statement`.

The big `for loop` loads opcode one by one, and the big `switch statement` executes different C code according to the opcode.

The `for loop` will check the variable `gil_drop_request` and release the `gil` if necessary.

Not every opcode will check the `gil_drop_request`. Some opcodes that end with `FAST_DISPATCH()` will go to the next statement directly, while some opcodes that end with `DISPATCH()` act as a `continue statement` and will go to the beginning of the for loop.

```c
/* cpython/Python/ceval.c */
main_loop:
    for (;;) {
        /* omit */
        if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.eval_breaker)) {
            opcode = _Py_OPCODE(*next_instr);
            if (opcode == SETUP_FINALLY ||
                opcode == SETUP_WITH ||
                opcode == BEFORE_ASYNC_WITH ||
                opcode == YIELD_FROM) {
                /* go to switch statement without check for the gil */
                goto fast_next_opcode;
            }
            /* omit */
            if (_Py_atomic_load_relaxed(
                        &_PyRuntime.ceval.gil_drop_request))
            {
            	/* if the gil_drop_request is set by other thread */
                /* Give another thread a chance */
                if (PyThreadState_Swap(NULL) != tstate)
                    Py_FatalError("ceval: tstate mix-up");
                drop_gil(tstate);

                /* Other threads may run now */

                take_gil(tstate);

                /* Check if we should make a quick exit. */
                if (_Py_IsFinalizing() &&
                    !_Py_CURRENTLY_FINALIZING(tstate))
                {
                    drop_gil(tstate);
                    PyThread_exit_thread();
                }

                if (PyThreadState_Swap(tstate) != NULL)
                    Py_FatalError("ceval: orphan tstate");
            }
            /* omit */
        }

    fast_next_opcode:
		/* omit */
    switch (opcode) {
        case TARGET(NOP): {
            FAST_DISPATCH();
        }
        /* omit */
        case TARGET(UNARY_POSITIVE): {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Positive(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }
    	/* omit */
    }
    /* omit */
}

```

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/ceval.png)