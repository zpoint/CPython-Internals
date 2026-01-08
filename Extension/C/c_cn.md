# python 性能分析和 C 扩展

# 目录

* [概览](#概览)
* [示例](#示例)
	* [性能分析](#性能分析)
	* [C 模块](#C-模块)
		* [python2](#python2)
		* [python3](#python3)
* [更多资料](#更多资料)

# 概览

最近我在做一个关于优化之前其他人写好的 API 业务接口的任务, 这个接口是在 Django 服务中编写的, 这个 API 会启动一个异步任务, 这个异步任务有时需要好几分钟才能结束, 在这个过程中用户会处于等待被通知的过程, 我通过了一些方法把40秒才能运行完成的任务压缩到了1秒左右

# 示例

## 性能分析

[line_profiler](https://github.com/rkern/line_profiler) 是一个很好的性能分析工具, 我们下面使用的是这个工具

下面的结果我是使用 `python2.7` 运行的

```python3
import line_profiler
import atexit
profile = line_profiler.LineProfiler()
atexit.register(profile.print_stats)

meaningless_dict = {chr(i) if i < 256 else i: i for i in range(500)}


@profile
def my_cpu_bound_task(x, y):
    """
    meaningless code
    """
    r = bytearray(1000)
    for i in range(1000):
        if i in meaningless_dict.keys():
            y += 1
        x += i
        y = (x % 256) ^ (y % 256)
        r[i] = y
    return bytes(r)


@profile
def run():
    for i in range(9999):
        my_cpu_bound_task(i, i+1)


if __name__ == "__main__":
    run()

```

注意, 如果你用如下的命令 `python manage.py runserver` 跑 django 服务, 默认情况下 `runserver` 会启动其他线程去执行你的请求, 此时会至少有两个线程注册了 `profile.print_stats`, 他们都定向到同一个标准输出的情况下, 你看到的结果可能是他们交织在一起无法阅读的结果

你需要在代码中手动调用 `profile.print_stats()` 输出结果, 并取消注册 `profile.print_stats`

这是输出

```shell script
Timer unit: 1e-06 s

Total time: 142.908 s
File: pro.py
Function: my_cpu_bound_task at line 9

Line #      Hits         Time  Per Hit   % Time  Line Contents
==============================================================
     9                                           @profile
    10                                           def my_cpu_bound_task(x, y):
    11                                               """
    12                                               meaningless code
    13                                               """
    14      9999      17507.0      1.8      0.0      r = bytearray(1000)
    15  10008999    2980894.0      0.3      2.1      for i in range(1000):
    16   9999000  128237346.0     12.8     89.7          if i in meaningless_dict.keys():
    17   2439756     868063.0      0.4      0.6              y += 1
    18   9999000    3400631.0      0.3      2.4          x += i
    19   9999000    4062262.0      0.4      2.8          y = (x % 256) ^ (y % 256)
    20   9999000    3310637.0      0.3      2.3          r[i] = y
    21      9999      30411.0      3.0      0.0      return bytes(r)

Total time: 156.807 s
File: pro.py
Function: run at line 24

Line #      Hits         Time  Per Hit   % Time  Line Contents
==============================================================
    24                                           @profile
    25                                           def run():
    26     10000       5612.0      0.6      0.0      for i in range(9999):
    27      9999  156801599.0  15681.7    100.0          my_cpu_bound_task(i, i+1)


```

我们可以发现 `if i in meaningless_dict.keys():` 花费了接近 128 秒钟去执行, 主要原因是 `dict.keys()` 会生成一个新的列表, 并把所有 `dict` 中的 `keys` 插入这个新生成的列表中, 并且 `in dict.keys()` 也会变成 `O(n)` 的列表搜索

对于 python3.x, `dict.keys()` 会返回一个 `dict_keys` 对象, 他只是一个空壳, 里面实际上装的还是指向 `dict` 的索引, 就不会产生上面的性能问题

如果我们把 `if i in meaningless_dict.keys():` 改成 `if i in meaningless_dict:`

```python3
Total time: 17.0502 s
File: pro.py
Function: my_cpu_bound_task at line 9

Line #      Hits         Time  Per Hit   % Time  Line Contents
==============================================================
     9                                           @profile
    10                                           def my_cpu_bound_task(x, y):
    11                                               """
    12                                               meaningless code
    13                                               """
    14      9999      15453.0      1.5      0.1      r = bytearray(1000)
    15  10008999    2868245.0      0.3     16.8      for i in range(1000):
    16   9999000    3188187.0      0.3     18.7          if i in meaningless_dict:
    17   2439756     756708.0      0.3      4.4              y += 1
    18   9999000    3014318.0      0.3     17.7          x += i
    19   9999000    3936202.0      0.4     23.1          y = (x % 256) ^ (y % 256)
    20   9999000    3241203.0      0.3     19.0          r[i] = y
    21      9999      29847.0      3.0      0.2      return bytes(r)


```

同一行的执行时间被压缩到了 3 秒钟以下, 花费了大概 18.6% 的时间, 但是还有其他的占用不少时间的代码, 我们是否可以做的更好一些呢 ?

## C 模块

我们可以在 C 中重写 **my_cpu_bound_task** 这个函数

你可以在 [python2.7-capi](https://docs.python.org/2.7/c-api/index.html) 或者 [python3.7-capi](https://docs.python.org/3.7/c-api/index.html) 找到所有你需要的 C API 文档

这里也有一个教程教你如何编写 C 扩展 [Writing a C Extension Module](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)

## python2

需要一个 [setup.py](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py2/my_mod/setup.py), 和一个 C 代码文件存储所有需要的功能函数, 我起名叫做 [my_module.c](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py2/my_mod/my_module.c)

我不打算复制粘贴所有的代码到 readme 中, 你可以在 [CPython-Internals/Extension/C/profile_py2/my_mod/](https://github.com/zpoint/CPython-Internals/tree/master/Extension/C/profile_py2/my_mod) 中找到全部代码

```shell script
# 只对 python2.x 生效
# python -m pip install ipython==5.8.0
# python -m pip install cython
# python -m pip install line_profiler

$ git clone https://github.com/zpoint/CPython-Internals.git
$ cd CPython-Internals/Extension/C/profile_py2/
$ python profile.py
# 运行需要大概 3 分钟, 产生和上面类似的结果
$ cd my_mod/
# 编译 C 模块
$ python setup.py build
# 下面的路径根据操作系统不同, 路径也会不同
$ ls build/
lib.macosx-10.14-intel-2.7	temp.macosx-10.14-intel-2.7
$ ls build/lib.macosx-10.14-intel-2.7/
# 这是我们需要的编译好的文件
my_module.so
$ cp build/lib.macosx-10.14-intel-2.7/my_module.so ../
$ cd ..
$ python profile_better.py
# 我们成功的把这个任务的时间压缩到了 1 秒钟以下
Timer unit: 1e-06 s

Total time: 0.363531 s
File: profile_better.py
Function: run at line 25

Line #      Hits         Time  Per Hit   % Time  Line Contents
==============================================================
    25                                           @profile
    26                                           def run():
    27     10000       3357.0      0.3      0.9      for i in range(9999):
    28      9999     360174.0     36.0     99.1          my_module.my_cpu_bound_task(i, i+1)

```

## python3

需要一个 [setup.py](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py3/my_mod/setup.py), 和一个 C 代码文件存储所有需要的功能函数, 我起名叫做 [my_module.c](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py3/my_mod/my_module.c)

我不打算复制粘贴所有的代码到 readme 中, 你可以在 [CPython-Internals/Extension/C/profile_py3/my_mod/](https://github.com/zpoint/CPython-Internals/tree/master/Extension/C/profile_py3/my_mod) 中找到全部代码

```shell script
# 只对 python3.x 生效
# python3 -m pip install cython
# git clone https://github.com/rkern/line_profiler.git
# find line_profiler -name '*.pyx' -exec python3 -m cython {} \;
# cd line_profiler
# python3 -m pip install . --user

$ git clone https://github.com/zpoint/CPython-Internals.git
$ cd CPython-Internals/Extension/C/profile_py3/
$ python3 profile_py3.py
# 运行需要大概 40 秒钟, 产生和上面类似的结果
$ cd my_mod/
# 编译 C 扩展模块
$ python3 setup.py build
# 下面的路径根据操作系统不同, 路径也会不同
$ ls build/
lib.macosx-10.14-x86_64-3.7	temp.macosx-10.14-x86_64-3.7
$ ls build/lib.macosx-10.14-x86_64-3.7/
# 这是我们需要的编译好的文件
my_module.cpython-37m-darwin.so
$ cp build/lib.macosx-10.14-x86_64-3.7/my_module.cpython-37m-darwin.so ../
$ cd ..
$ python3 profile_better.py
# 我们成功的把这个任务的时间压缩到了 1 秒钟以下
Timer unit: 1e-06 s

Total time: 0.338438 s
File: profile_better.py
Function: run at line 25

Line #      Hits         Time  Per Hit   % Time  Line Contents
==============================================================
    25                                           @profile
    26                                           def run():
    27     10000       3169.0      0.3      0.9      for i in range(9999):
    28      9999     335269.0     33.5     99.1          my_module.my_cpu_bound_task(i, i+1)


```

# 更多资料

* [python2.7-capi](https://docs.python.org/2.7/c-api/index.html)
* [python3.7-capi](https://docs.python.org/3.7/c-api/index.html)
* [python line profiler without magic](https://lothiraldan.github.io/2018-02-18-python-line-profiler-without-magic/)
* [Writing a C Extension Module for Python](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)
