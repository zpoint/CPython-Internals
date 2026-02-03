# tuple

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [how element stored inside](#how-element-stored-inside)
* [free list](#free-list)

# related file
* cpython/Include/cpython/tupleobject.c
* cpython/Include/tupleobject.h
* cpython/Objects/tupleobject.c

# memory layout

![memory layout](https://img-blog.csdnimg.cn/20190313121821367.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

The structure of the **tuple** object is simpler than other Python objects.
Obviously, **ob_item** is an array of PyObject* pointers, and all elements will be stored inside the **ob_item** array. But how exactly is each element stored in **PyTupleObject**? Does the first element begin at index 0? What is the resize strategy?

Let's see

# how element stored inside

```python3
t = tuple()

```

![tuple_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_empty.png)

```python3
t = ("aa", )

```

![tuple_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_1.png)

```python3
t = ("aa", "bb", "cc", "dd")

```

**ob_size** represents the size of the **PyTupleObject** object. Because a tuple object is immutable, **ob_item** is the start address of an array of pointers to **PyObject**, and the size of this array is **ob_size**. There's no need for a resize operation.

![tuple_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_4.png)

# free list

The free_list mechanism used here is more interesting than [free_list in list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list.md#delete-and-free-list)

```c
#define PyTuple_MAXSAVESIZE     20
#define PyTuple_MAXFREELIST  2000
static PyTupleObject *free_list[PyTuple_MAXSAVESIZE];
static int numfree[PyTuple_MAXSAVESIZE];


```

Let's examine what **free_list** and **numfree** are.
We assume that the Python interpreter doesn't create/deallocate any tuple objects internally during the following code

```python3
>>> t = tuple()
>>> id(t)
4458758208
>>> del t
>>> t = tuple()
>>> id(t) # same as previous deleted one
4458758208
>>> t2 = tuple()
>>> id(t2) # same as t
4458758208

```

![delete_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_0.png)

```python3
>>> t = ("aa", )
>>> id(t)
4459413360
>>> del t
>>> t = ("bb", )
>>> id(t) # it's not repeatable, because I assume that the Python interpreter doesn't create/deallocate any tuple objects during execution
4459413360
>>> t2 = ("cc", )
>>> del t
>>> del t2

```

![delete_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_2.png)

num_free[i] indicates how many objects are left in free_list[i]. When you create a new tuple with size i, CPython will use the top object at free_list[i]

```python3
>>> t = ("aa", )

```

![delete_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_3.png)

```python3
>>> t2 = ("aa", "bb")
>>> t3 = ("cc", "dd")
>>> t4 = ("ee", "ff")
>>> t5 = ("gg", "hh")
>>> del t2
>>> del t3
>>> del t4
>>> del t5

```

![delete_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/delete_4.png)
