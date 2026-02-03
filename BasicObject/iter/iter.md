# iter

# contents

* [related file](#related-file)
* [iterator](#iterator)
    * [memory layout](#memory-layout-iter)
    * [example](#example)
        * [iter0](#iter0)
        * [iter1](#iter1)
        * [iter2](#iter2)
        * [iter3](#iter3)
        * [iter4](#iter4)
        * [iter5](#iter5)
        * [iter end](#iter-end)

* [callable iterator](#callable-iterator)
    * [memory layout](#memory-layout-citer)
    * [example](#example-citer)
        * [citer0](#citer0)
        * [citer1](#citer1)
        * [citer-end](#citer-end)

# related file
* cpython/Objects/iterobject.c
* cpython/Include/iterobject.h

# iterator

## memory layout iter

The Python sequence iterator is a wrapper of a Python object with _\_getitem_\_ defined.

So the sequence iterator can iterate through the target object by calling object[index], and set index to index + 1

```c
static PyObject *
iter_iternext(PyObject *iterator)
{
    seqiterobject *it;
    PyObject *seq;
    PyObject *result;

    assert(PySeqIter_Check(iterator));
    it = (seqiterobject *)iterator;
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    if (it->it_index == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "iter index too large");
        return NULL;
    }

    result = PySequence_GetItem(seq, it->it_index);
    if (result != NULL) {
        it->it_index++;
        return result;
    }
    if (PyErr_ExceptionMatches(PyExc_IndexError) ||
        PyErr_ExceptionMatches(PyExc_StopIteration))
    {
        PyErr_Clear();
        it->it_seq = NULL;
        Py_DECREF(seq);
    }
    return NULL;
}

```

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/layout.png)

## example

### iter0

Let's iterate through an iterator object

```python3
class A(object):
    def __getitem__(self, index):
        if index >= 5:
            print("raise by myself")
            raise IndexError
        if index == 0:
            return "index 0"
        elif index == 1:
            return ["index 1", "good boy"]
        return index * 2 if index < 3 else index * 3

a = iter(A())
type(a) # iterator

```

![iter0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter0.png)

### iter1

**next(a)** calls object[0] and returns the result

```python3
>>> next(a)
'index 0'

```

![iter1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter1.png)

### iter2

```python3
>>> next(a)
['index 1', 'good boy']

```

![iter2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter2.png)

### iter3

The current **it_index** is 2, so next(a) calls object[2] which returns 4

```python3
>>> next(a)
4

```

![iter3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter3.png)

### iter4

```python3
>>> next(a)
9

```

![iter4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter4.png)

### iter5

```python3
>>> next(a)
12

```

![iter5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter5.png)

### iter end

Now the **it_index** is 5, and next(a) will raise an IndexError.

The content in **it_index** is still 5, but the content in **it_seq** becomes 0x00, which indicates end of iteration

```python3
>>> next(a)
raise by myself
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![iterend](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iterend.png)

Notice: if you call next(a) again, **"raise by myself"** won't be printed, since the **it_seq** field no longer points to the instance of **class A**. It has lost the way to access the instance of **class A** and can no longer call the _\_getitem_\_ function

```python3
>>> next(a)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration


```

# callable iterator

## memory layout citer

![callable_layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/callable_layout.png)

```c
static PyObject *
calliter_iternext(calliterobject *it)
{
    PyObject *result;

    if (it->it_callable == NULL) {
        return NULL;
    }

    result = _PyObject_CallNoArg(it->it_callable);
    if (result != NULL) {
        int ok;

        ok = PyObject_RichCompareBool(it->it_sentinel, result, Py_EQ);
        if (ok == 0) {
            return result; /* Common case, fast path */
        }

        Py_DECREF(result);
        if (ok > 0) {
            Py_CLEAR(it->it_callable);
            Py_CLEAR(it->it_sentinel);
        }
    }
    else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        PyErr_Clear();
        Py_CLEAR(it->it_callable);
        Py_CLEAR(it->it_sentinel);
    }
    return NULL;
}

```

A callable_iterator calls whatever is stored in **it_callable** each time you iterate through it, until the result matches **it_sentinel** or a StopIteration is raised


## example citer

### citer0

```python3
class A(object):
    def __init__(self):
        self.val = 0

    def __call__(self, *args, **kwargs):
        self.val += 1
        # if self.val == 3:
        #     raise StopIteration
        return self.val

r = iter(A(), 2)
type(r) # callable_iterator

```

![citer0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citer0.png)

### citer1

The callable_iterator calls the _\_call_\_ method of the instance of A and compares the result with the sentinel **2**.

The next operation won't change any state of the callable_iterator object; the state inside **it_callable** is changed instead

```python3
>>> next(r)
1

```

![citer1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citer1.png)

### citer end

This time the instance of A returns PyLongObject 2, which is the same as the object stored in the **it_sentinel** field.

So the callable_iterator clears its state and returns NULL to the outer scope, which raises a StopIteration

```python3
>>> next(r)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
StopIteration

```

![citerend](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citerend.png)

