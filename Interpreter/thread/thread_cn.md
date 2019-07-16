# thread

# 目录

* [相关位置文件](#相关位置文件)
* [阅读须知](#阅读须知)
* [内存构造](#内存构造)
* [start_new_thread](#start_new_thread)
* [allocate_lock](#allocate_lock)
* [allocate_rlock](#allocate_rlock)
* [exit_thread](#exit_thread)
* [stack_size](#thread_stack_size)

# 相关位置文件

* cpython/Modules/_threadmodule.c
* cpython/Python/thread.c
* cpython/Python/thread_nt.h
* cpython/Python/thread_pthread.h
* cpython/Python/pystate.c
* cpython/Include/cpython/pystate.h

# 阅读须知

以下的内容展示的是 CPython 实现线程的时候使用的是哪些相关的系统函数, 你可以通过下文了解到比如 "posix 信号量 还是 posix 互斥锁 被用在了 python 线程锁的实现中 ?", 如果你的疑问是 "**posix 线程** 是什么 ?  **posix 信号量** 是什么 ? 你应该先参考 [APUE](https://www.amazon.cn/dp/B01AG3ZVOA) 的 第 11 和 第 13 章, 还有 [UNP 卷 2](https://www.amazon.cn/dp/B01CK7JI44)

如果你感兴趣的是 线程的 c 结构体表示, 以及线程如何组织的, 接下来会有一篇 [概览](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/overview/overview_cn.md) 来说明

# 内存构造

一个 bootstate 结构体存储了一个 python 线程所需要的一切信息

![bootstate](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/bootstate.png)

**thread.c** 会根据编译时的系统环境, 选择导入 **thread_nt.h** 或者 **thread_pthread.h**

**thread_nt.h** 和 **thread_pthread.h** 都定义了相同的线程相关的 API 函数接口, 调用这些函数的时候会转发到对应的系统环境的系统调用上

![thread](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/thread.png)

下面的内容展现的是 **thread_pthread.h** 中定义的 posix 部分, 对于其他平台(比如 windows nt), 你可以直接查看对应的代码, 即使系统调用的接口不同, 但封装后对外的接口是相同的

注意, 不推荐代码中直接使用 `_thread` 模块的方式调用, 它是 C 语言写好的直接暴露出来的接口, 偏底层, 下面的代码实例仅供展示说明

你应该使用 `threading` 等封装了更丰富的功能的模块


# example

    from threading import Thread, currentThread, activeCount

    def my_func(a, b, c=4):
        print(a, b, c, currentThread().ident, activeCount())

    Thread(target=my_func, args=("hello world", "Who are you")).start()

**threading** 提供了更丰富的线程相关的功能, 它是用 python 实现的集成在标准库中的对 **_thread** 的封装, 你可以直接读它的代码实现

# start_new_thread

**PyEval_InitThreads** 会在之前没有创建过的情况下创建 [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_cn.md)

由于在 python3.2 线程切换策略的升级, 单线程的情况下无需循环的释放并且重新获取 [gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_cn.md)

	/* cpython/Modules/_threadmodule.c */
    static PyObject *
    thread_PyThread_start_new_thread(PyObject *self, PyObject *fargs)
    {
        PyObject *func, *args, *keyw = NULL;
        struct bootstate *boot;
        unsigned long ident;

        /* 忽略一些细节
        从 fargs 中提取出 func, args, keyw, 并且检查他们的类型
        如果你跑的是上面的例子, 那么提取出的几个元素如下所示
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
        /* PyEval_InitThreads 会检查是否需要创建 gil */
        PyEval_InitThreads();
        /* 找到对应的系统函数并调用, 在我的平台上是 pthread_create */
        ident = PyThread_start_new_thread(t_bootstrap, (void*) boot);
        if (ident == PYTHREAD_INVALID_THREAD_ID) {
            /* 处理错误 */
        }
        return PyLong_FromUnsignedLong(ident);
    }

# allocate_lock

这是一个普通的线程锁

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

在 posix 系统中, **lock_lock** 的创建过程如下

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

从上面的代码中我们可以发现, CPython 会优先使用 [posix 信号量](http://www.csc.villanova.edu/~mdamian/threads/posixsem.html) 来实现 **lock_lock**, 如果系统不支持则使用 [posix 互斥锁](http://www.skrenta.com/rt/man/pthread_mutex_init.3.html) 来实现, 下面的部分用 信号量来展示

	>>> r.acquire_lock()

如果你尝试获取一个线程锁, 根据你传入的 timeout 的值的不同, 会调用对应的不同的系统函数


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
		/* 如果被 signal 中断了, 如果调用者需要被通知到这个 signal, 通知它, 如果不需要, 继续等待 */
        if (intr_flag || status != EINTR) {
            break;
        }

        if (microseconds > 0) {
            /* 被 signal 中断了 (EINTR): 重新计算 timeout 值 */
        }
    }

如果你成功的获取到了这个线程锁, locked 上的值会变为 1

![lock_object_locked](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/lock_object_locked.png)

# allocate_rlock

**rlock** 是递归互斥锁的简称, 如 [维基百科](https://zh.wikipedia.org/wiki/%E5%8F%AF%E9%87%8D%E5%85%A5%E4%BA%92%E6%96%A5%E9%94%81) 所说

> 计算机科学中，可重入互斥锁（英语：reentrant mutex）是互斥锁的一种，同一线程对其多次加锁不会产生死锁。可重入互斥锁也称递归互斥锁（英语：recursive mutex）或递归锁（英语：recursive lock）

![rlock_object](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object.png)

	>>> r = _thread.RLock()

创建 rlock 结构体中的 **lock_lock** 的过程和上面创建普通锁的过程类似

在获取锁之前, **rlock_owner** 和 **rlock_count** 都会被设置成 0

	>>> r.acquire()

![rlock_object_acquire](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object_acquire.png)

	>>> r.acquire()

![rlock_object_acquire2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/thread/rlock_object_acquire2.png)

我们发现 **rlock_owner** 的值为当前线程的 ident, **rlock_count** 是一个计数器, 表示这把锁当前被几个调用者持有中

获取锁的过程非常简明

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
        	/* 之前已经获得过了, 直接增加 rlock_count 上的值即可 */
            unsigned long count = self->rlock_count + 1;
            if (count <= self->rlock_count) {
                PyErr_SetString(PyExc_OverflowError,
                                "Internal lock count overflowed");
                return NULL;
            }
            self->rlock_count = count;
            Py_RETURN_TRUE;
        }
        /* 调用对应的 posix 系统调用去获得这把锁 */
        r = acquire_timed(self->rlock_lock, timeout);
        if (r == PY_LOCK_ACQUIRED) {
        	/* 如果当前没有其他线程获得过这把锁, 并且自己之前也没有获得过, 那么获取成功后进入这里 */
            assert(self->rlock_count == 0);
            self->rlock_owner = tid;
            self->rlock_count = 1;
        }
        else if (r == PY_LOCK_INTR) {
        	/* 如果这把锁正被其他线程持有中, 则无法获得, 进入这里 */
            return NULL;
        }
        return PyBool_FromLong(r == PY_LOCK_ACQUIRED);
    }


锁的释放过程

	/* cpython/Modules/_threadmodule.c */
    static PyObject *
    rlock_release(rlockobject *self, PyObject *Py_UNUSED(ignored))
    {
        unsigned long tid = PyThread_get_thread_ident();
        /* 检查 rlock_count 的值是否变为了 0, 或者是否是其他线程进行的释放 */
        if (self->rlock_count == 0 || self->rlock_owner != tid) {
            PyErr_SetString(PyExc_RuntimeError,
                            "cannot release un-acquired lock");
            return NULL;
        }
        /* 减小 rlock_count 的值,
        如果 rlock_count 变为 0, 把 rlock_owner 也设置为 0, 这样其他线程也能获得这把锁 */
        if (--self->rlock_count == 0) {
            self->rlock_owner = 0;
            PyThread_release_lock(self->rlock_lock);
        }
        Py_RETURN_NONE;
    }

# exit_thread

	/* cpython/Modules/_threadmodule.c */
    static PyObject *
    thread_PyThread_exit_thread(PyObject *self, PyObject *Py_UNUSED(ignored))
    {
    	/* 抛出一个 SystemExit 的异常 */
        PyErr_SetNone(PyExc_SystemExit);
        return NULL;
    }

# stack_size

你可以更改当前线程的 stack 空间大小, 这个空间是由操作系统控制的

	# 设置为 100 kb
	>>> _thread.stack_size(102400) # threading.stack_size(102400)

最后会调用到对应的 [`pthread_attr_setstacksize`](http://man7.org/linux/man-pages/man3/pthread_attr_setstacksize.3.html) 这个系统函数

    _pythread_pthread_set_stacksize(size_t size)
    {
    #if defined(THREAD_STACK_SIZE)
        pthread_attr_t attrs;
        size_t tss_min;
        int rc = 0;
    #endif

        /* 设为默认值 */
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
            /* 调用对应的系统函数去设置这个值, 看能不能设置成功 */
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
