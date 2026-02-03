# C++ extension

# contents

* [overview](#overview)
* [example](#example)
* [integrate with NumPy](#integrate-with-NumPy)
* [bypass the GIL](#bypass-the-GIL)
* [read more](#read-more)

# overview

We've written a [C extension](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c.md) before to improve performance

Now we want to take a further step to develop a complex module in C++ and expose it as a Python API

The C++ compiler is compatible with C and I want to use the power of `STL` in `C++11`(or newer) standard

# example

```python3
# setup.py
from distutils.core import setup, Extension

my_module = Extension('m_example', sources=['./example.cpp'], extra_compile_args=['-std=c++11'])

setup(name='m_example',
      version='1.0',
      description='my module to use C++ STL',
      ext_modules=[my_module])
```

and the [example.cpp](https://github.com/zpoint/CPython-Internals/blob/master/Extension/CPP/example/example.cpp) get elements from the Python array and stores all integer to a C++ vectors and use `std::sort` from `<algorithm>` to sort the vector, finally returns the first element as Python object to the caller

Run the example

```bash
cd Cpython-Internals/Extension/CPP/example
python3 setup.py build
mv build/lib.macosx-10.15-x86_64-3.8/m_example.cpython-38-darwin.so ./
zpoint@zpoints-MacBook-Pro example % python3
Python 3.8.4 (default, Jul 14 2020, 02:58:48)
[Clang 11.0.3 (clang-1103.0.32.62)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
>>> import m_example
>>> m_example.example([6,4,3])
3
>>>
```

# integrate with NumPy

Sometimes we need to work with NumPy, let's begin with an example

We take a two-dimensional NumPy array, a one-dimensional NumPy array as input, and a double value, do some computation, and write the result back to the one-dimensional array

The numpy array should have dtype float64

two_dim is `3*N` array

one_dim is `N` array

```python3
import numpy as np
def compute(two_dim: np.array, one_dim: np.array, val: np.float) -> None:
	# do some computation and store result to one_dim
	for index in range(len(one_dim)):
		one_dim[index] = (two_dim[0][index] + two_dim[1][index] + two_dim[2][index]) * val 
	# ...
```

We are going to write the above function as extension module in C++ 

[example.cpp](https://github.com/zpoint/CPython-Internals/blob/master/Extension/CPP/m_numpy/example.cpp)

```bash
cd Cpython-Internals/Extension/CPP/m_numpy
python3 setup.py build
mv build/lib.macosx-10.15-x86_64-3.8/m_example.cpython-38-darwin.so ./
zpoint@zpoints-MacBook-Pro m_numpy % python3
Python 3.8.4 (default, Jul 14 2020, 02:58:48) 
[Clang 11.0.3 (clang-1103.0.32.62)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
>>> import numpy as np
>>> import m_example
>>> one_dim = np.zeros([2], dtype=np.float)
>>> two_dim = np.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]], dtype=np.float)
>>> m_example.example(two_dim, one_dim, 0.5)
>>> one_dim
array([4.5, 6. ])
```

# bypass the GIL

What if we want to schedule a parallel task to utilize the runtime of our task?

two_dim becomes `X * 3 * N` array

one_dim is `X * N` array

```python3
import numpy as np
def compute(two_dim: np.array, one_dim: np.array, val: np.float) -> None:
	# do some computation and store result to one_dim
	for task_index in range(len(one_dim)):
		curr_one_dim = one_dim[task_index]
		curr_two_dim = two_dim[task_index]
	for index in range(len(one_dim)):
		curr_one_dim[index] = (curr_two_dim[0][index] + curr_two_dim[1][index] + curr_two_dim[2][index]) * val
	# ...
```

I want to separate these tasks into several different threads, and let the OS schedule them to work together

We've learned about the [GIL](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil.md) before. We know that the interpreter in different threads will acquire the mutex before executing every Python bytecode, so as long as our code in different threads is not executed by the Python interpreter, everything will be fine

We use the `std::future` from `C++ STL` to schedule our thread according to the environment variable `CONCURRENCY_NUM`

```bash
cd Cpython-Internals/Extension/CPP/m_parallel
python3 setup.py build
mv build/lib.macosx-10.15-x86_64-3.8/m_example.cpython-38-darwin.so ./
zpoint@zpoints-MacBook-Pro m_parallel % python3
Python 3.8.4 (default, Jul 14 2020, 02:58:48) 
[Clang 11.0.3 (clang-1103.0.32.62)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
>>> import os
>>> import numpy as np
>>> import m_example
>>> os.putenv("CONCURRENCY_NUM", "2")
>>> one_dim = np.zeros([4, 2], dtype=np.float)
>>> two_dim = np.array([[[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]], [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]], [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]], [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]]], dtype=np.float)
>>> m_example.example(two_dim, one_dim, 0.5)
>>> print(one_dim)
[[4.5 6. ]
 [4.5 6. ]
 [4.5 6. ]
 [4.5 6. ]]
```



# read more

* [numpy-api](https://numpy.org/doc/stable/reference/c-api/array.html?highlight=array%20api)
