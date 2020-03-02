# complex

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)

# 相关位置文件
* cpython/Objects/complexobject.c
* cpython/Include/complexobject.h
* cpython/clinic/complexobject.c.h

# 内存构造

**PyComplexObject** 内部存储了两个双精度浮点数, 处理过程和表示方法都和 [float](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float_cn.md) 对象很类似

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/layout.png)

# 示例

```python3
c = complex(0, 1)

```

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example0.png)

```python3
d = complex(0.1, -0.2)

```

![example1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example1.png)

我们来读下加法部分的代码

```c
Py_complex
_Py_c_sum(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real + b.real;
    r.imag = a.imag + b.imag;
    return r;
}

static PyObject *
complex_add(PyObject *v, PyObject *w)
{
    Py_complex result;
    Py_complex a, b;
    TO_COMPLEX(v, a); // 检查下类型, 如果不为 PyComplexObject 需要先转换为 PyComplexObject
    TO_COMPLEX(w, b);
    PyFPE_START_PROTECT("complex_add", return 0) // 在 3.7 版本之后就没用了
    result = _Py_c_sum(a, b); // sum the complex
    PyFPE_END_PROTECT(result) // 在 3.7 版本之后就没用了
    return PyComplex_FromCComplex(result);
}

```

加法看起来比较简单, 把实数 **real** 部分相加, 把虚数 **imag** 部分相加, 并返回相加后的整个结构

加减乘数, 负号, 平方等操作都很类似

```python3
>>> e = c + d
>>> repr(e)
'(0.1+0.8j)'

```

![example2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example2.png)

