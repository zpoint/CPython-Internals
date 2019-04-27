# bytearray

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [empty bytearray](#empty-bytearray)
	* [ascii characters](#ascii-characters)
	* [nonascii characters](#nonascii-characters)
* [summary](#summary)
	* [ob_shash](#ob_shash)
	* [ob_size](#ob_size)
	* [summary](#summary)

#### related file
* cpython/Objects/bytearrayobject.c
* cpython/Include/bytearrayobject.h
* cpython/Objects/clinic/bytearrayobject.c.h

#### memory layout

The **ob_alloc** field represent the real allocated size in bytes

**ob_bytes** is the physical begin address, and **ob_start** is the logical begin address

**ob_exports** means how many other object are sharing this buffer, like reference count in a way

![memory layout](https://img-blog.csdnimg.cn/20190315152551189.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)


#### example

##### empty bytearray

	>>> a = bytearray(b"")
    >>> id(a)
    4353755656
    >>> b = bytearray(b"")
    >>> id(b) # they are not shared
    4353755712


![empty](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytearray/empty.png)

##### append

after append a charracter 'a', **ob_alloc** becomes 2, **ob_bytes** and **ob_start** all points to same address

	a.append(ord('a'))

![append_a](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytearray/append_a.png)

##### resize

	a.append(ord('b'))

![resize](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytearray/resize.png)

