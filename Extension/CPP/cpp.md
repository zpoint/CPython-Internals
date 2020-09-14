# C++ extension![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/cpp_ext)

# contents

* [overview](#overview)
* [example](#example)
* [integrate with NumPy](#integrate-with-NumPy)
* [bypass the GIL](#bypass-the-GIL)
* [read more](#read-more)

# overview

We've written [C extension](https://github.com/zpoint/CPython-Internals/blob/master/Extension/C/c.md) before to help performance penalty

Now we want to take a further step to develop a complex module in C++ and expose to python API

The C++ compiler is compatible with C and I want to use the power of STL in C++11(or newer) standard

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

and the [example.cpp](https://github.com/zpoint/CPython-Internals/blob/master/Extension/CPP/example/example.cpp) 

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



# read more

* [python2.7-capi](https://docs.python.org/2.7/c-api/index.html)
* [python3.7-capi](https://docs.python.org/3.7/c-api/index.html)
* [python line profiler without magic](https://lothiraldan.github.io/2018-02-18-python-line-profiler-without-magic/)
* [Writing a C Extension Module for Python](http://madrury.github.io/jekyll/update/programming/2016/06/20/python-extension-modules.html)
