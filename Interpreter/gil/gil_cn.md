# gil

### 目录

* [相关位置文件](#相关位置文件)
* [介绍](#介绍)
	* [python3.2-之前的线程切换](#python32-之前的线程切换)
	* [python3.2-之后的线程切换](#python3.2-之后的线程切换)
	* [内存构造](#内存构造)
* [字段](#字段)
	* [interval](#interval)
	* [last_holder](#last_holder)
	* [locked](#locked)
	* [switch_number](#switch_number)
	* [mutex](#mutex)
	* [cond](#cond)
	* [switch_cond and switch_mutex](#switch_cond-and-switch_mutex)
* [gil何时会被释放](#gil何时会被释放)


#### 相关位置文件

* cpython/Python/ceval.c
* cpython/Python/ceval_gil.h
* cpython/Include/internal/pycore_gil.h

#### 介绍

这是 [**Global Interpreter Lock**](https://wiki.python.org/moin/GlobalInterpreterLock) 的定义(我来翻译一下)

> 在 CPython 中, 全局解释器锁, 或者 GIL, 是一把互斥锁, 这把互斥锁被用来保护 python 对象, 防止多个线程同时执行 python 字节码. 这把锁是有存在的必要的, 主要原因是 CPython 的内存管理机制在实现的时候是非线程安全的(因为GIL的存在, 很多第三方的扩展和功能在写的时候都深度的依赖这把锁, 更加加深了对 GIL 的依赖)

##### python32 之前的线程切换

通俗的来讲, **tick** 是一个计数器, 表示当前线程在释放 **gil** 之前连续的执行了多少个字节码(实际上有部分执行较快的字节码并不会被计入计数器)

如果当前的线程正在执行一个 CPU 密集型的任务, 它会在 **tick** 计数器到达 100 之后就释放 **gil**, 给其他线程一个获得 **gil** 的机会

如果当前的线程正在执行一个 IO 密集型的任务, 你执行 `sleep/recv/send(...etc)` 这些会阻塞的系统调用时, 即使 **tick** 计数器的值还没到 100, **gil** 也会被主动地释放

你可以调用 `sys.setcheckinterval()` 这个函数把 **tick** 计数器的值从默认的 100 改为其他的值

![old_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/old_gil.png)
(图片来自 [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

因为 **tick** 并不是以时间为基准计数, 而是以 opcode 个数为基准的计数, 有一些 opcode 代码复杂耗时长, 一些耗时短, 进而导致同样的 100 个 **tick**, 一些线程的执行时间总是执行的比另一些线程长

在多核机器上, 如果两个线程都在执行 CPU 密集型的任务, 操作系统有可能让这两个线程在不同的核心上运行, 也许会出现以下的情况, 当一个拥有了 **gil** 的线程在一个核心上执行 100 次 **tick** 的过程中, 在另一个核心上运行的线程频繁的进行抢占 **gil**, 抢占失败的循环, 导致 CPU 瞎忙影响性能

当前的实现完全的把任务(线程)调度交给了操作系统, 哪个线程会抢到锁被唤醒, 哪个线程抢不到锁被阻塞, 程序员是无法控制的, 这也就导致了以下情况发生的概率, 想象一下一个处理 IO 密集型的线程已经收到了 IO 的信号, 但是需要等待另一个线程释放 **gil** 才能去处理, 在等待的过程中, 另一个线程执行满了 **tick** 次数, 释放了 **gil**, 但是之后自己又抢到了自己释放的 **gil**, 导致前面的 IO 密集型的线程又需要至少多等待 100 个 **tick** 才能处理已经接收到的信号, 有可能在信号丢失前都无法及时处理(实际上, 自己主动触发操作系统进行任务调度的线程会比被操作系统强制触发任务调度的线程在执行队列里有更高的优先级, 程序员可以让 IO 密集型任务尽快的进入等待状态, 主动触发任务调度, 提高这个线程的优先级)(详情参考操作系统相关资料)

![gil_battle](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_battle.png)
(图片来自 [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

##### python3.2-之后的线程切换

由于上面列出的多核机器下可能导致的性能等方面的影响, **gil** 的实现在 python3.2 之后进行了一些优化

如果当前只有一个线程, 那么这个线程会永远执行下去, 无需检查并释放 **gil**

如果当前有不止一个线程, 当前等待 **gil** 的线程在超过一定时间的等待后, 会把全局变量 **gil_drop_request** 的值设置为 1, 之后继续等待相同的时间, 这时拥有 **gil** 的线程看到了 **gil_drop_request** 变为 1, 就会主动释放 **gil** 并通过 condition variable 通知到在等待中的线程, 第一个被唤醒的等待中的线程会抢到 **gil** 并执行相应的任务

![new_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)
(图片来自 [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

注意, 把 **gil_drop_request** 设置为 1 的线程不一定是抢到 **gil** 的线程

![new_gil2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil2.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

如果你对 **gil** 的详细介绍感兴趣, 请参考 [Understanding the Python GIL(article)](http://www.dabeaz.com/GIL/)

##### 内存构造

![git_layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_layout.png)

#### 字段

python 解释器本质上是一个 C 程序, 所有的可执行的 C 程序都有 `main` 函数的入口, python 解释器也不例外

你可以在 `cpython/Modules/main.c` 找到和 `main` 函数相关的部分, 通过这部分函数你可以发现, 在执行 `main loop` 之前, 解释器做了很多相关变量的初始化, 其中就包括创建 `_gil_runtime_state` 和初始化里面的值

	./python.exe

![init](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/init.png)

##### interval

    >>> import sys
    >>> sys.getswitchinterval()
    0.005

**interval** 是线程在设置 `gil_drop_request` 这个变量之前需要等待的时长(单位微秒), 5000 微秒 等价于 0.005 秒

在 C 里面是用 微秒 为单位存储, 在 python 解释器中以秒来表示

##### last_holder

**last_holder** 存放了最后一个持有 **gil** 的线程的 C 中对应的 PyThreadState 结构的指针地址, 通过这个值我们可以知道当前线程释放了 **gil** 后, 是否有其他线程获得了 **gil**(可以采取措施避免被自己重新获得)

##### locked

**locked** 的类型为 **_Py_atomic_int**, 值 -1 表示还未初始化, 0 表示当前的 **gil** 处于释放状态, 1 表示某个线程已经占用了 **gil**, 这个值的类型设置为原子类型之后在 `ceval.c` 就可以不加锁的对这个值进行读取

	/* cpython/Python/ceval_gil.h */
    static void take_gil(PyThreadState *tstate)
    {
        /* 忽略 */
        /* 这个位置已经获得了 GIL */
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 1);
        _Py_ANNOTATE_RWLOCK_ACQUIRED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);
        if (tstate != (PyThreadState*)_Py_atomic_load_relaxed(
                        &_PyRuntime.ceval.gil.last_holder))
        {
            _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                     (uintptr_t)tstate);
            ++_PyRuntime.ceval.gil.switch_number;
        }
        /* 忽略 */
    }

    static void drop_gil(PyThreadState *tstate)
    {
        /* 忽略 */
        if (tstate != NULL) {
            _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                     (uintptr_t)tstate);
        }
        MUTEX_LOCK(_PyRuntime.ceval.gil.mutex);
        _Py_ANNOTATE_RWLOCK_RELEASED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 0);
        /* 忽略 */
    }

##### switch_number

**switch_number** 是一个计数器, 表示从解释器运行到现在, **gil** 总共被释放获得多少次

在函数 `take_gil` 中使用到

    static void take_gil(PyThreadState *tstate)
    {
        /* 忽略 */
        while (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked)) {
        	/* 只要 gil 是锁住的状态, 进入这个循环 */
            int timed_out = 0;
            unsigned long saved_switchnum;

            saved_switchnum = _PyRuntime.ceval.gil.switch_number;
            /* 释放 gil.mutex, 并待 INTERVAL 微秒(默认 5000) 或者等待过程中收到 gil.cond 的信号 */
            COND_TIMED_WAIT(_PyRuntime.ceval.gil.cond, _PyRuntime.ceval.gil.mutex,
                            INTERVAL, timed_out);
            /* 当前持有 gil.mutex 这把互斥锁 */
            if (timed_out &&
                _Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked) &&
                _PyRuntime.ceval.gil.switch_number == saved_switchnum) {
                /* 如果超过了等待时间, 并且这段等待时间里没有进行 gil 的换手, 则让当前持有 gil 的线程进行释放
                把 gil_drop_request 值设为 1 */
                SET_GIL_DROP_REQUEST();
            }
            /* 继续回到 while 循环, 检查 gil 是否为锁住状态 */
        }
        /* 忽略 */
    }

##### mutex

**mutex** 是一把互斥锁, 用来保护 `locked`, `last_holder`, `switch_number` 还有 `_gil_runtime_state` 中的其他变量

##### cond

**cond** 是一个 condition variable, 和 **mutex** 结合起来一起使用, 当前线程释放 **gil** 时用来给其他等待中的线程发送信号

##### switch_cond and switch_mutex

**switch_cond** 是另一个 condition variable, 和 **switch_mutex** 结合起来可以用来保证释放后重新获得 **gil** 的线程不是同一个前面释放 **gil** 的线程, 避免 **gil** 换手但是线程未切换浪费 cpu 时间

这个功能如果编译时未定义 `FORCE_SWITCHING` 则不开启

    static void drop_gil(PyThreadState *tstate)
    {
    /* 忽略 */
    #ifdef FORCE_SWITCHING
        if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil_drop_request) &&
            tstate != NULL)
        {
        	/* 如果 gil_drop_request 已经设置了并且 tstate 不为空 */
            /* 锁住 switch_mutex 这把互斥锁 */
            MUTEX_LOCK(_PyRuntime.ceval.gil.switch_mutex);
            if (((PyThreadState*)_Py_atomic_load_relaxed(
                        &_PyRuntime.ceval.gil.last_holder)
                ) == tstate)
            {
            /* 如果 last_holder 是当前线程, 释放 switch_mutex 这把互斥锁, 等待 switch_cond 这个条件变量的信号 */
            RESET_GIL_DROP_REQUEST();
            	/* 注意, 如果 COND_WAIT 不在互斥锁释放后原子的启动,
                另一个线程有可能会在这中间拿到 gil 并释放,
                '并且重置这个条件变量, 这个过程发生在了 COND_WAIT 之前 */
                COND_WAIT(_PyRuntime.ceval.gil.switch_cond,
                          _PyRuntime.ceval.gil.switch_mutex);
        }
            MUTEX_UNLOCK(_PyRuntime.ceval.gil.switch_mutex);
        }
    #endif
    }

#### gil何时会被释放

`cpython/Python/ceval.c` 中的 `main_loop` 是一个很大的 `for loop`, 并且其中含有一个很大的 `switch statement`

这个很大的 `for loop` 会按顺序逐个的加载 opcode, 并委派给中间很大的 `switch statement` 去进行执行, `switch statement` 会根据不同的 opcode 跳转到不同的位置执行

`for loop` 在开始位置会检查 `gil_drop_request` 变量, 必要的时候会释放 `gil`

不是所有的 opcode 执行之前都会检查 `gil_drop_request` 的, 有一些 opcode 结束时的代码为 `FAST_DISPATCH()`, 这部分 opcode 会直接跳转到下一个 opcode 对应的代码的部分进行执行

而另一些 `DISPATCH()` 结尾的作用和 `continue` 类似, 会跳转到 `for loop` 顶端, 重新检测 `gil_drop_request`, 必要时释放 `gil`

	/* cpython/Python/ceval.c */
    main_loop:
        for (;;) {
 			/* 忽略 */
            if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.eval_breaker)) {
                opcode = _Py_OPCODE(*next_instr);
                if (opcode == SETUP_FINALLY ||
                    opcode == SETUP_WITH ||
                    opcode == BEFORE_ASYNC_WITH ||
                    opcode == YIELD_FROM) {
                    /* 跳过 gil 部分, 直接跳转到 switch 部分 */
                    goto fast_next_opcode;
                }
                /* 忽略 */
                if (_Py_atomic_load_relaxed(
                            &_PyRuntime.ceval.gil_drop_request))
                {
                	/* 如果 gil_drop_request 被其他线程设置为 1 */
                    /* 给其他线程一个获得 gil 的机会 */
                    if (PyThreadState_Swap(NULL) != tstate)
                        Py_FatalError("ceval: tstate mix-up");
                    drop_gil(tstate);

                    /* 其他线程现在在运行中 */

                    take_gil(tstate);

                    /* 检查是否需要退出 */
                    if (_Py_IsFinalizing() &&
                        !_Py_CURRENTLY_FINALIZING(tstate))
                    {
                        drop_gil(tstate);
                        PyThread_exit_thread();
                    }

                    if (PyThreadState_Swap(tstate) != NULL)
                        Py_FatalError("ceval: orphan tstate");
                }
                /* 忽略 */
            }

        fast_next_opcode:
			/* 忽略 */
        switch (opcode) {
            case TARGET(NOP): {
                FAST_DISPATCH();
            }
            /* 忽略 */
            case TARGET(UNARY_POSITIVE): {
                PyObject *value = TOP();
                PyObject *res = PyNumber_Positive(value);
                Py_DECREF(value);
                SET_TOP(res);
                if (res == NULL)
                    goto error;
                DISPATCH();
            }
        	/* 忽略 */
        }
        /* 忽略 */
    }

![ceval](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/ceval.png)

