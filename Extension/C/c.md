# profile python code and write pure C extension

# contents

* [overview](#overview)
* [example](#example)
	* [profile](#profile)
	* [C module](#C-module)
		* [python2](#python2)
		* [python3](#python3)
* [read more](#read-more)

# overview

recently I get a task to improve someone else's API performance written in Django server, the API will start an async job and in some cases last several minutes, I manage to improve a 40 seconds CPU bound task to lower than 1 second

# example

## profile

[line_profiler](https://github.com/rkern/line_profiler) is a handful tool to begin with

I am running with `python2.7`

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

notice, if you're running django server with `python manage.py runserver`, the default behaviour of `runserver` will spawn a thread to handle your request, there will be at least two thread register the `profile.print_stats`, their output may interleave together and become confused

you may need to call `profile.print_stats()` manually in your code without register the `profile.print_stats`

this is the output

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

we can see that `if i in meaningless_dict.keys():` takes about 128 seconds to run, this is mainly because `dict.keys()` will generate a new list like object and insert all the `keys` in the `dict` into the newly generated list, and `in dict.keys()` will become a `O(n)` list search

for python3.x, `dict.keys()` will return a `dict_keys` object, which is a view of the `dict`, the performance issue will be gone

if we change `if i in meaningless_dict.keys():` to `if i in meaningless_dict:`

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

the runtime of the same line becomes about 3 seconds, it consumes about 18.6% runtime of the function, there still exist other time consuming line, can we do better ?

## C module

we can rewrite the **my_cpu_bound_task** in C

you can find all the C API you need from [python2.7-capi](https://docs.python.org/2.7/c-api/index.html) or [python3.7-capi](https://docs.python.org/3.7/c-api/index.html)

you can also refer to guide of [Writing a C Extension Module](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)

## python2

a [setup.py](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py2/my_mod/setup.py) is needed, and a source code file stores all the C function, I name it [my_module.c](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py2/my_mod/my_module.c)

I am not going to copy pasted all the C source code in the readme

it's avaliable in [CPython-Internals/Extension/C/profile_py2/my_mod/](https://github.com/zpoint/CPython-Internals/tree/master/Extension/C/profile_py2/my_mod)

```shell script
# only available for python2.x
# python -m pip install ipython==5.8.0
# python -m pip install cython
# python -m pip install line_profiler

$ git clone https://github.com/zpoint/CPython-Internals.git
$ cd CPython-Internals/Extension/C/profile_py2/
$ python profile.py
# it takes about 3 minutes to run, the output will be similar to above
$ cd my_mod/
# build the C extension module
$ python setup.py build
# the following directory is system dependent, you should copy the file according to your system
$ ls build/
lib.macosx-10.14-intel-2.7	temp.macosx-10.14-intel-2.7
$ ls build/lib.macosx-10.14-intel-2.7/
# this is the compiled file we need
my_module.so
$ cp build/lib.macosx-10.14-intel-2.7/my_module.so ../
$ cd ..
$ python profile_better.py
# we manage to improve the runtime of the task to lower than 1 second
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

a [setup.py](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py3/my_mod/setup.py) is needed, and a source code file stores all the C function, I name it [my_module.c](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py3/my_mod/my_module.c)

I am not going to copy pasted all the C source code in the readme

it's avaliable in [CPython-Internals/Extension/C/profile_py3/my_mod/](https://github.com/zpoint/CPython-Internals/tree/master/Extension/C/profile_py3/my_mod)

```shell script
# only available for python3.x
# python3 -m pip install cython
# git clone https://github.com/rkern/line_profiler.git
# find line_profiler -name '*.pyx' -exec python3 -m cython {} \;
# cd line_profiler
# python3 -m pip install . --user

$ git clone https://github.com/zpoint/CPython-Internals.git
$ cd CPython-Internals/Extension/C/profile_py3/
$ python3 profile_py3.py
# it takes about 40 seconds to run, the output will be similar to above
$ cd my_mod/
# build the C extension module
$ python3 setup.py build
# the following directory is system dependent, you should copy the file according to your system
$ ls build/
lib.macosx-10.14-x86_64-3.7	temp.macosx-10.14-x86_64-3.7
$ ls build/lib.macosx-10.14-x86_64-3.7/
# this is the compiled file we need
my_module.cpython-37m-darwin.so
$ cp build/lib.macosx-10.14-x86_64-3.7/my_module.cpython-37m-darwin.so ../
$ cd ..
$ python3 profile_better.py
# we manage to improve the runtime of the task to lower than 1 second
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

# read more

* [python2.7-capi](https://docs.python.org/2.7/c-api/index.html)
* [python3.7-capi](https://docs.python.org/3.7/c-api/index.html)
* [python line profiler without magic](https://lothiraldan.github.io/2018-02-18-python-line-profiler-without-magic/)
* [Writing a C Extension Module for Python](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)
