# iter

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [iter0](#iter0)
	* [iter1](#iter1)
	* [iter2](#iter2)
	* [iter3](#iter3)
	* [iter4](#iter4)
	* [iter5](#iter5)
	* [iter end](#iter-end)

#### related file
* cpython/Objects/iterobject.c
* cpython/Include/iterobject.h

#### memory layout

The python sequence iterator is a wrapper of a python object with _\_getitem_\_ defined, so the sequence iterator can iter through the target object by calling the object[index], and set index to index + 1

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

![layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/layout.png)

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

![iter0](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter0.png)

##### iter1

the **next(a)** calls a[0], and return the result

	>>> next(a)
	'index 0'

![iter1](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter1.png)

##### iter2

	>>> next(a)
	['index 1', 'good boy']

![iter2](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter2.png)

##### iter3

the current **it_index** is 2, so the next(a) calls a[2] which returns 4

	>>> next(a)
	4

![iter3](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter3.png)

##### iter4

    >>> next(a)
    9

![iter4](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter4.png)

##### iter5

    >>> next(a)
    12

![iter5](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter5.png)

##### iter end

now, the **it_idnex** is 5, next(a) will raise an IndexError

the content in **it_index** is still 5, but the content in **it_seq** becomes 0x00, which indicate end of iteration

    >>> next(a)
    raise by myself
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![iterend](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iterend.png)

notice, if you call next(a) again, the **"raise by myself"** won't be printed, since the **it_seq** field no longer points to the instance of **class A**, it lose the way to access the instance of **class A**, and can no longer call the _\_getitem_\_ function

    >>> next(a)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

