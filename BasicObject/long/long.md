# tuple

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [how element stored inside](#how-element-stored-inside)
* [free list](#free-list)

#### related file
* cpython/Objects/longobject.c
* cpython/Include/longobject.h
* cpython/Include/longintrepr.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190314164305131.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

After python3, there's only one type named **int**, the **long** type in python2.x is **int** type in python3.x

The structure of **long object** looks like the structure of [tuple object](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/tuple/tuple.md#memory-layout), Obviously there's only one field to store the real **int** value, that's **ob_digit**. But how cpython represent the variable size **int** in byte level?

Let's see

#### how element stored inside

	i = 0

![tuple_empty](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/tuple/tuple_empty.png)

