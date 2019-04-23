# bytes

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [empty bytes](#empty-bytes)
	* [add](#add)
		* [hash collision](#hash-collision)
		* [resize](#resize)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES)
	* [clear](#clear)

#### related file
* cpython/Objects/bytesobject.c
* cpython/Include/bytesobject.h
* cpython/Objects/clinic/bytesobject.c.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190318160629447.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

The memory layout of **PyBytesObject** looks like [memory layout of tuple object](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/tuple/tuple.md#memory-layout) and [memory layout of int object](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/long/long.md#memory-layout), but simpler than any of them.

#### example

##### empty bytes

**bytes** object is an immutable object, whenever you need to modify a **bytes** object, you need to create a new one, which keeps the implementation simple.

s = ""