# bytes

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [empty bytes](#empty-bytes)
	* [ascii characters](#ascii-characters)
	* [nonascii characters](#nonascii-characters)
* [summary](#summary)
	* [ob_shash](#ob_shash)
	* [ob_size](#ob_size)
	* [summary](#summary)

#### related file
* cpython/Objects/bytesobject.c
* cpython/Include/bytesobject.h
* cpython/Objects/clinic/bytesobject.c.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190318160629447.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

The memory layout of **PyBytesObject** looks like [memory layout of tuple object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#memory-layout) and [memory layout of int object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long.md#memory-layout), but simpler than any of them.

#### example

##### empty bytes

**bytes** object is an immutable object, whenever you need to modify a **bytes** object, you need to create a new one, which keeps the implementation simple.

	s = b""

![empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/empty.png)

##### ascii characters

let's initialize a byte object with ascii characters

	s = b"abcdefg123"

![ascii](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/ascii.png)

##### nonascii characters

	s = "我是帅哥".encode("utf8")

![nonascii](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/nonascii.png)

#### summary


##### ob_shash


The field **ob_shash** should stores the hash value of the byte object, value **-1** means not computed yet.

The first time the hash value computed, it will be cached to the **ob_shash** field

the cached hash value can saves recalculation and speeds up dict lookups

##### ob_size

field **ob_size** is inside every **PyVarObject**, the **PyBytesObject** uses this **field** to store size information to keep O(1) time complexity for **len()** opeeration and tracks the size of non-ascii string(may be null characters inside)

##### summary

The **PyBytesObject** is a python wrapper of c style null terminate string, with **ob_shash** for caching hash value and **ob_size** for storing the size information of **PyBytesObject**

The implementation of **PyBytesObject** looks like the **embstr** encoding in redis

	redis-cli
    127.0.0.1:6379> set a "hello"
    OK
    127.0.0.1:6379> object encoding a
    "embstr"
