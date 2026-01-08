# complex

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)

# related file
* cpython/Objects/complexobject.c
* cpython/Include/complexobject.h
* cpython/clinic/complexobject.c.h

# memory layout

the **PyComplexObject** stores two double precision floating point number inside

the handling process and representation are mostly the same as [float](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/float/float.md) object

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/layout.png)

# example

```python3
c = complex(0, 1)

```

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example0.png)

```python3
d = complex(0.1, -0.2)

```

![example1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example1.png)

let's read the add function

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
    TO_COMPLEX(v, a); // check the type, covert to complex if type is not complex
    TO_COMPLEX(w, b);
    PyFPE_START_PROTECT("complex_add", return 0) // useless after version 3.7
    result = _Py_c_sum(a, b); // sum the complex
    PyFPE_END_PROTECT(result) // useless after version 3.7
    return PyComplex_FromCComplex(result);
}

```

the add operation is quite simple, sum the **real**, sum the **image** part and return the new value

the sub/divide/pow/neg operations are similar

```python3
>>> e = c + d
>>> repr(e)
'(0.1+0.8j)'

```

![example2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/complex/example2.png)

