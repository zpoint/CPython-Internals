# gil

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [create](#create)

#### related file

* cpython/Python/ceval.c
* cpython/Python/ceval_gil.h
* cpython/Include/internal/pycore_gil.h

#### memory layout

this is the defination of the **Global Interpreter Lock**

> In CPython, the global interpreter lock, or GIL, is a mutex that protects access to Python objects, preventing multiple threads from executing Python bytecodes at once. This lock is necessary mainly because CPython's memory management is not thread-safe. (However, since the GIL exists, other features have grown to depend on the guarantees that it enforces.)

thread scheduling before python3.2

basically, the **tick** is a counter for how many opcodes current thread executed continuously without releasing the **gil**

if the current thread is running a CPU-bound task, it will release the **gil** and offer an opportunity for other thread to run for every 100 **ticks**

if the current thread is running an IO-bound task, the **gil** will be relaesed manually if you call `sleep/recv/send(...etc)` even without count to 100 **ticks**

you can call `sys.setcheckinterval()` to set other **tick** count value instead of 100

![old_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/old_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

because the **tick** is not time-based, some thread might run far longer than other threads

in multi-core machine, if two threads both running CPU-bound tasks, the os might schedule the two threads running on different cores, there might be a situation that one thread holding the **gil** executing it's task in it's **100 ticks cycle** in a core, while the thread in the other core wakes up preiodly try to acquire the **gil** but fail, spinning the CPU

![gil_battle](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_battle.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

due to some performance issue in multi-core machine, the implementation of the **GIL** has changed a lot after python3.2

thread scheduling after python3.2

if there's only one thread, it can run forever without check and release **gil**

if there're more than one threads, the thread currently blocking by the **gil** will wait or period of timeout and set the **gil_drop_request** to 1, and continue waiting, the thread currently holding the **gil** will release it if the **gil_drop_request** is set to 1, and thread currently blocking will be signaled and is able to acqure the **gil**

![new_gil](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

the thread set the **gil_drop_request** to 1 may not be the thread acquire the **gil**

![new_gil2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/new_gil.png)
(picture from [Understanding the Python GIL(youtube)](https://www.youtube.com/watch?v=Obt-vMVdM8s))

for those who are interested in detail, please refer to [Understanding the Python GIL(article)](http://www.dabeaz.com/GIL/)

![git_layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_layout.png)

#### create

the python intepreter in a program written in C, every executable program written in C have a `main` function

those `main` related functions are defined in `cpython/Modules/main.c`, you will find that the `main` related function does some inilialization for the intepreter status before execute the `main loop`, the `_gil_runtime_state` will be created and initialized in the inilialization

	./python.exe
    >>> import sys
    >>> sys.getswitchinterval()
    0.005

interval is the suspend timeout before set the `gil_drop_request` microseconds

![init](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/init.png)



