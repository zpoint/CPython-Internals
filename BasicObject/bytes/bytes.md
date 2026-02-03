# bytes

# contents

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

# related file
* cpython/Objects/bytesobject.c
* cpython/Include/bytesobject.h
* cpython/Objects/clinic/bytesobject.c.h

# memory layout

![memory layout](https://img-blog.csdnimg.cn/20190318160629447.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

The memory layout of **PyBytesObject** looks like [memory layout of tuple object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#memory-layout) and [memory layout of int object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/long/long.md#memory-layout), but simpler than any of them.

# example

## empty bytes

**bytes** object is an immutable object, whenever you need to modify a **bytes** object, you need to create a new one, which keeps the implementation simple.

```python3
s = b""

```

![empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/empty.png)

## ascii characters

Let's initialize a bytes object with ASCII characters

```python3
s = b"abcdefg123"

```

![ascii](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/ascii.png)

## nonascii characters

```python3
s = "我是帅哥".encode("utf8")

```

![nonascii](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytes/nonascii.png)

# summary


## ob_shash


The field **ob_shash** should store the hash value of the byte object, value **-1** means not computed yet.

The first time the hash value is computed, it will be cached in the **ob_shash** field.

The cached hash value can save recalculation and speed up dictionary lookups

## ob_size

The field **ob_size** is inside every **PyVarObject**. **PyBytesObject** uses this field to store size information to keep O(1) time complexity for the **len()** operation and to track the size of non-ASCII strings (which may contain null characters inside)

## summary

The **PyBytesObject** is a Python wrapper of C-style null-terminated strings, with **ob_shash** for caching the hash value and **ob_size** for storing size information

The implementation of **PyBytesObject** looks like the **embstr** encoding in redis

```shell script
redis-cli
127.0.0.1:6379> set a "hello"
OK
127.0.0.1:6379> object encoding a
"embstr"

```

