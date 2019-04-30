# iter

### 目录

* [相关位置文件](#相关位置文件)
* [iterator](#iterator)
    * [内存构造](#内存构造iter)
    * [示例](#示例)
        * [iter0](#iter0)
        * [iter1](#iter1)
        * [iter2](#iter2)
        * [iter3](#iter3)
        * [iter4](#iter4)
        * [iter5](#iter5)
        * [iter end](#iter-end)

* [callable iterator](#callable-iterator)
    * [内存构造](#内存构造citer)
    * [示例](#示例citer)
    	* [citer0](#citer0)
    	* [citer1](#citer1)
    	* [citer-end](#citer-end)

### related file
* cpython/Objects/iterobject.c
* cpython/Include/iterobject.h

### iterator

#### 内存构造iter

python 中的sequence迭代器是一层包装, 包装的内容是一个定义了 _\_getitem_\_ 方法的 python 对象

所以 sequence 迭代器可以通过调用 object[index] 来迭代这个目标对象, 在迭代的过程中会把 index 设置为 index + 1

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

#### 示例

##### iter0

我们来尝试迭代一个 sequence 迭代器

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

**next(a)** 调用了方法 object[0], 并返回了对应的结果

	>>> next(a)
	'index 0'

![iter1](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter1.png)

##### iter2

	>>> next(a)
	['index 1', 'good boy']

![iter2](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iter2.png)

##### iter3

当前的 **it_index** 为 2, 所以 next(a) 会调用 object[2] 并返回 4

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

现在 **it_idnex** 为 5, next(a) 会抛出一个 IndexError

**it_index** 中存储的值仍为 5, 但是 **it_seq** 中存储的指针则变为了 0x00 这个空指针, 通过这种方式来表示迭代结束

    >>> next(a)
    raise by myself
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![iterend](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/iterend.png)

注意, 如果你再次调用 next(a), **"raise by myself"** 是不会被打印出来的, 当前的 **it_seq** 已经指向了空的指针, 这个迭代器已经丢失了对迭代对象的指针/引用 了, 你没有办法再找到这个被迭代的对象并调用这个对象上面对应的方法

    >>> next(a)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration


### callable iterator

#### 内存构造citer

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

一个 callable 迭代器 在每次迭代的过程中会调用 **it_callable** 里面存储的对象, 直到这个对象返回了和 **it_sentinel** 相同的值或者自己抛出 StopIteration 为止

![callable_layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/callable_layout.png)


#### 示例 citer

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

![citer0](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/citer0.png)

##### citer1

callable 迭代器 调用了 A 的实例的 _\_call_\_ 方法, 并且用返回值和 **sentinal** 的值 2 做了比较

下一次调用 next 并不会改变 callable 迭代器 内存储的任何状态, 只有 存储在 **it_callable** 的这个对象本身发生了一些改变

    >>> next(r)
    1

![citer1](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/citer1.png)

##### citer end

这次 A 的实例返回了 **PyLongObject** 对象, 值为 2, 和存储在 **it_sentinel** 中的值相同,

所以 callable 迭代器 会清空他自己当前的状态, 并返回 NULL 给上一级调用者, 上一级调用者会抛出 StopIteration

    >>> next(r)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    StopIteration

![citerend](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/iter/citerend.png)

