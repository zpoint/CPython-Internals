# C++ extension![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/cpp_ext)

# 目录

* [介绍](#介绍)
* [示例](#示例)
* [和 NumPy 结合](#和-NumPy-结合)
* [bypass the GIL](#bypass-the-GIL)
* [read more](#read-more)

# 介绍

我们在之前已经尝试过使用 [C 扩展](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c_cn.md) 来提高性能了, 现在我们尝试更进一步, 在 C++ 下编写一个扩展, 并同样的利用 python API 暴露给 python 调用

C++ 的编译器是兼容 C 语言的, 我们将会借助 `C++11`(或以上) 的标准的 `STL` 来实现一些功能

# 示例

```python3
# setup.py
from distutils.core import setup, Extension

my_module = Extension('m_example', sources=['./example.cpp'], extra_compile_args=['-std=c++11'])

setup(name='m_example',
      version='1.0',
      description='my module to use C++ STL',
      ext_modules=[my_module])
```

[example.cpp](https://github.com/zpoint/CPython-Internals/blob/master/Extension/CPP/example/example.cpp) 从 Python 数组中获取元素, 之后把它作为整数类型存储在了 `C++` 的 `vector` 中, 然后用 `<algorithm>` 中的 `std::sort` 方法对其进行排序, 最后把排好序的第一个值作为 Python 对象返回给调用者

运行这个示例

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

# 和 NumPy 结合

有些时候我们需要和 NumPy 结合使用, 让我们从一个新的示例开始

我们以一个二维数组, 一个一维数组, 和一个双精度浮点数作为输入, 对输入进行一些运算, 之后把结果写回一维数组中

numpy 数组中的元素类型我们先指定为 float64

two_dim 维度是 `3*N`

one_dim 维度是 `N`

```python3
import numpy as np
def compute(two_dim: np.array, one_dim: np.array, val: np.float) -> None:
	# do some computation and store result to one_dim
	for index in range(len(one_dim)):
		one_dim[index] = (two_dim[0][index] + two_dim[1][index] + two_dim[2][index]) * val 
	# ...
```

我们会把上面的示例函数用 C++ 扩展的方式进行重写

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

# 绕过 GIL

如果我们想要通过并行任务的方式, 来缩小我们任务的执行时间呢 ?

two_dim 的维度变成了 `X * 3 * N`

one_dim 的维度变成了 `X * N`

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

我想要把这些任务均匀地分布到不同的线程中, 并让操作系统对这些任务进行调度

我们先前已经学过了 [GIL](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gil/gil_cn.md) 相关的知识了, 我们知道不同线程中的解释器会在执行 python 字节码之前获取同一个互斥锁, 所以只要我们在不同线程中的代码不通过解释器去执行即可

我们这里会使用到 `C++ STL` 中的 `std::future` 来让我们不同的线程执行任务(具体起多少个 `future` 根据环境变量 `CONCURRENCY_NUM` 来判断)

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


# 更多资料

* [numpy-api](https://numpy.org/doc/stable/reference/c-api/array.html?highlight=array%20api)
