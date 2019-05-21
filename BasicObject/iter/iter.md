# iter

### contents

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

### related file
* cpython/Objects/iterobject.c
* cpython/Include/iterobject.h

### iterator

#### memory layout iter

The python sequence iterator is a wrapper of a python object with _\_getitem_\_ defined

so the sequence iterator can iter through the target object by calling the object[index], and set index to index + 1

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

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/layout.png)

#### example

##### iter0

let's iter through an iterator object

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

![iter0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter0.png)

##### iter1

the **next(a)** calls object[0], and return the result

    >>> next(a)
    'index 0'

![iter1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter1.png)

##### iter2

    >>> next(a)
    ['index 1', 'good boy']

![iter2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter2.png)

##### iter3

the current **it_index** is 2, so the next(a) calls object[2] which returns 4

    >>> next(a)
    4

![iter3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter3.png)

##### iter4

    >>> next(a)
    9

![iter4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter4.png)

##### iter5

    >>> next(a)
    12

![iter5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iter5.png)

##### iter end

now, the **it_idnex** is 5, next(a) will raise an IndexError

the content in **it_index** is still 5, but the content in **it_seq** becomes 0x00, which indicate end of iteration

    >>> next(a)
    raise by myself
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![iterend](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/iterend.png)

notice, if you call next(a) again, the **"raise by myself"** won't be printed, since the **it_seq** field no longer points to the instance of **class A**, it loses the way to access the instance of **class A**, and can no longer call the _\_getitem_\_ function

    >>> next(a)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration


### callable iterator

#### memory layout citer

![callable_layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/callable_layout.png)

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

a callable_iterator calls whatever stores in **it_callable** each time you iter through it, until the result match the **it_sentinel** or a StopIteration raised


#### example citer

##### citer0

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

![citer0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citer0.png)

##### citer1

the callable_iterator calls the _\_call_\_ method of the instance of A, and compare the result with the sentinel **2**

the next operation won't change any state of the callable_iterator object, the state inside **it_callable** is changed anyway

    >>> next(r)
    1

![citer1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citer1.png)

##### citer end

this time the instance of A returns PyLongObject 2, which is the same as the object stored inside **it_sentinel** field,

so the callable_iterator clears its state and return NULL to the outer scope, which raise a StopIteration

    >>> next(r)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![citerend](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/iter/citerend.png)

