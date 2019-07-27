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

recently I get a task to improve someone else's API written in Django server, the API will start a job and in some cases last several minutes, I manage to improve a 60 seconds task to about 20 seconds

# example

## profile

[line_profiler](https://github.com/rkern/line_profiler) is a handful tool to begin with

I am running with `python2.7`

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

notice, if you're running django server with `python manage.py runserver`, the default behaviour of `runserver` will spawn a thread to handle your request, there will be at least two thread register the `profile.print_stats`, the output may interleave together and become confused

you may need to call `profile.print_stats()` manually in your code and wihout register the `profile.print_stats`

this is the output

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



we can see that `if i in meaningless_dict.keys():` takes about 128 seconds to run, this is mainly because `dict.keys()` will generate a new list like object and insert all the `keys` in the `dict` into the newly generated list, and `in dict.keys()` will become a `O(n)` list search

for python3.x, `dict.keys()` will return a `dict_keys` object, which is a view of the `dict`, the performance issue will be gone

if we change `if i in meaningless_dict.keys():` to `if i in meaningless_dict:`

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

the runtime of the same line becomes about 3 seconds, it consumes about 18.6% runtime of the function, there still exist other heave calculated line, can we do better ?

## C module

we can rewrite the **my_cpu_bound_task** in C

you can find all the C API you need from [python2.7-capi](https://docs.python.org/2.7/c-api/index.html) or [python3.7-capi](https://docs.python.org/3.7/c-api/index.html)

you can also refer to guide of [Writing a C Extension Module](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)

## python2

I am not going to copy pasted all the C source code in the readme

it's avaliable in [python2 profile](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/profile_py2/)




## python3

# read more

* [Garbage collection in Python: things you need to know](https://rushter.com/blog/python-garbage-collector/)
* [the garbage collector](https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/)
