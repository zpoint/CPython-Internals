# enum

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
    * [normal](#normal)
    * [en_longindex](#en_longindex)

# related file
* cpython/Objects/enumobject.c
* cpython/Include/enumobject.h
* cpython/Objects/clinic/enumobject.c.h

# memory layout

**enumerate** is a **type**. The instance of an **enumerate** object is an iterable object; you can iterate through the real delegated object while counting the index at the same time

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/layout.png)

# example

## normal

```python3
def gen():
    yield "I"
    yield "am"
    yield "handsome"

e = enumerate(gen())

>>> type(e)
<class 'enumerate'>

```

Before iterating through the object **e**, the **en_index** field is 0, **en_sit** stores the actual generator object being iterated, and **en_result** points to a tuple object with two empty values.

We will see the meaning of **en_longindex** later

![example0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example0.png)

```python3
>>> t1 = next(e)
>>> t1
(0, 'I')
>>> id(t1)
4469348888

```

Now the **en_index** becomes 1. The tuple in **en_result** is the last tuple object returned. The elements in the tuple are changed, but the address in **en_result** doesn't change, and this is not because of the [free-list mechanism in tuple](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#free-list).

It's a trick in the **enumerate** iterating function

```c
static PyObject *
enum_next(enumobject *en)
{
    /* omit */
    PyObject *result = en->en_result;
    /* omit */
    if (result->ob_refcnt == 1) {
        /* reference count of the tuple object is 1
          the only count is from the current enumerate object
          since we no longer need the old two element in tuple
          we can reset it instead of creating a new one
          */
        Py_INCREF(result);
        old_index = PyTuple_GET_ITEM(result, 0);
        old_item = PyTuple_GET_ITEM(result, 1);
        PyTuple_SET_ITEM(result, 0, next_index);
        PyTuple_SET_ITEM(result, 1, next_item);
        Py_DECREF(old_index);
        Py_DECREF(old_item);
        return result;
    }
    /*
    reach here, there are other reference to the old tuple
    we must create a new one instead of reset it
    */
    result = PyTuple_New(2);
    if (result == NULL) {
        Py_DECREF(next_index);
        Py_DECREF(next_item);
        return NULL;
    }
    PyTuple_SET_ITEM(result, 0, next_index);
    PyTuple_SET_ITEM(result, 1, next_item);
    return result;
}

```

It's clear: because only the current enumerate object keeps a reference to the old `tuple object -> (None, None)` object.

**enum_next** resets the 0th element in the tuple to 0 and the 1st element in the tuple to 'I', so the address **en_result** points to is the same

![example1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example1.png)

Because the reference count of the tuple object `(0, 'I') # id(4469348888)` now becomes 2 (one from the enumerate object and one from variable t1), the **enum_next** function will create and return a new tuple instead of resetting the one stored in **en_result**.

The **en_index** is incremented, and **en_result** still points to the old tuple object `(0, 'I') # id(4469348888)`

```python3
>>> next(e)
(1, 'am')

```

![example2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example2.png)

after `del t1`, the reference count of tuple object `(0, 'I') # id(4469348888)` becomes 1, so **enum_next** will reset the tuple **en_result** pointed to and returns it again

```python3
>>> del t1 # decrement the reference count of the object referenced by t1
>>> next(e)
(2, 'handsome')

```

![example3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example3.png)

The termination state is indicated by the object inside the **en_sit** field; nothing changed in the **enumerate** object

```python3
>>> next(e)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![example3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/example3.png)

## en_longindex

Usually, the **index** of an enumerate object is stored in the **en_index** field. **en_index** is of type **Py_ssize_t** in C, and **Py_ssize_t** is defined as

```c
#ifdef HAVE_SSIZE_T
typedef ssize_t         Py_ssize_t;
#elif SIZEOF_VOID_P == SIZEOF_SIZE_T
typedef Py_intptr_t     Py_ssize_t;
#else
#   error "Python needs a typedef for Py_ssize_t in pyport.h."
#endif

```

Most of the time it's a **ssize_t**, which is type **int** in 32-bit OS and **long int** in 64-bit OS.

On my machine, it's type **long int**.

What if the index is so big that a signed 64-bit integer can't hold it?

```python3
e = enumerate(gen(), 1 << 62)

```

The **en_index** can hold the value

![longindex0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/longindex0.png)

The max value **en_index** can represent is ((1 << 63) - 1) (PY_SSIZE_T_MAX).

Now the actual index is larger than PY_SSIZE_T_MAX, so **en_longindex** is used to represent the actual index.

What **en_longindex** points to is a [PyLongObject (Python type int)](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long.md), which can represent variable-size integers

```python3
>>> e = enumerate(gen(), (1 << 63) + 100)

```

![longindex1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/enum/longindex1.png)

