# gil

### contents

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

#### related file

* cpython/Python/ceval.c
* cpython/Python/ceval_gil.h
* cpython/Include/internal/pycore_gil.h

#### introduction

this is the defination of the [**Global Interpreter Lock**](https://wiki.python.org/moin/GlobalInterpreterLock)

> In CPython, the global interpreter lock, or GIL, is a mutex that protects access to Python objects, preventing multiple threads from executing Python bytecodes at once. This lock is necessary mainly because CPython's memory management is not thread-safe. (However, since the GIL exists, other features have grown to depend on the guarantees that it enforces.)

##### thread scheduling before python32

basically, the **tick** is a counter for how many opcodes current thread executed continuously without releasing the **gil**

if the current thread is running a CPU-bound task, it will release the **gil** and offer an opportunity for other thread to run for every 100 **ticks**

if the current thread is running an IO-bound task, the **gil** will be relaesed manually if you call `sleep/recv/send(...etc)` even without count to 100 **ticks**

you can call `sys.setcheckinterval()` to set other **tick** count value instead of 100

![old_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/old_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

because the **tick** is not time-based, some thread might run far longer than other threads

in multi-core machine, if two threads both running CPU-bound tasks, the os might schedule the two threads running on different cores, there might be a situation that one thread holding the **gil** executing it's task in it's **100 ticks cycle** in a core, while the thread in the other core wakes up preiodly try to acquire the **gil** but fail, spinning the CPU

the job(thread) schedule mechanism is fully controlled by the operating system, the thread handling IO-bound task have to wait for other thread to release the **gil**, and other thread might re-acquire the **gil** after it release the **gil**, which makes the current IO-bound task thread wait even longer (actually, the thread that cause os's context-switch by it self will have higher priority than those thread forced by os, programmer can utilize this feature by put the IO-bound thread to sleep as soon as possible)

![gil_battle](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_battle.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

##### thread scheduling after python32

due to some performance issue in multi-core machine, the implementation of the **gil** has changed a lot after python3.2

if there's only one thread, it can run forever without check and release **gil**

if there're more than one threads, the thread currently blocking by the **gil** will wait for a period of timeout and set the **gil_drop_request** to 1, and continue waiting, the thread currently holding the **gil** will release the **gil** and wait for same period of timeout if the **gil_drop_request** is set to 1, thread currently blocking will be signaled and is able to acqure the **gil**

![new_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

the thread set the **gil_drop_request** to 1 might not be the thread acquire the **gil**

if the current thread is waiting for the interval, and owner of the **gil** changed during the waiting **interval**, after wake up, the current thread need to set **gil_drop_request** to 1 and wait again

![new_gil2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil2.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

for those who are interested in detail, please refer to [Understanding the Python GIL(article)](http://www.dabeaz.com/GIL/)

##### memory layout

![git_layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_layout.png)

#### fields

the python intepreter in a program written in C, every executable program written in C have a `main` function

those `main` related functions are defined in `cpython/Modules/main.c`, you will find that the `main` related function does some inilialization for the intepreter status before execute the `main loop`, the `_gil_runtime_state` will be created and initialized in the inilialization

	./python.exe

![init](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/init.png)

##### interval

    >>> import sys
    >>> sys.getswitchinterval()
    0.005

**interval** is the suspend timeout before set the `gil_drop_request` in microseconds, 5000 microseconds is 0.005 seconds

it's stored as microseconds in C and represent as seconds in python

##### last_holder

**last_holder** stores the C address of the last PyThreadState hloding the **gil**, this helps us know whether anyone else was scheduled after we dropped the **gil**

##### locked

**locked** is a field of type **_Py_atomic_int**, -1 indicate uninitialized, 0 means no one is currently holding the **gil**, 1 means someone is holding it. This is atomic because it can be read without any lock taken in ceval.c

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

##### switch_number

**switch_number** is a counter for number of **gil** switches since the beginning

it's used in function `take_gil`

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

##### mutex

**mutex** is a mutex used for protecting `locked`, `last_holder`, `switch_number` and other variables in `_gil_runtime_state`

##### cond

**cond** is a condition variable, combined with **mutex**, used for signaling release of **gil**

##### switch_cond and switch_mutex

**switch_cond** is another condition variable, combined with **switch_mutex** can be used for making sure that the thread acquire the **gil** is not the thread release the **gil**, avoiding waste of time slice

it can be turned off without the defination of `FORCE_SWITCHING`

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

#### when will the gil be released

the `main_loop` in `cpython/Python/ceval.c` is a big `for loop`, and a big `switch statement`

the big `for loop` loads opcode one by one, and the big `switch statement` execute different c code according to the opcode

the `for loop` will check the variable `gil_drop_request` and release the `gil` if necessary

not every opcode will check the `gil_drop_request`, some opcode ends with `FAST_DISPATCH()` will go to the next statement directly, while some opcode ends with `DISPATCH()` act as `continue statement` and will go to the beginning of the for loop

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

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/ceval.png)