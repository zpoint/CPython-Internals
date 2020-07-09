# bytearray![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/bytearray_cn)

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
    * [empty bytearray](#empty-bytearray)
    * [append](#append)
    * [resize](#resize)
    * [slice](#slice)
* [ob_exports/buffer protocol](#ob_exports)

# related file
* cpython/Objects/bytearrayobject.c
* cpython/Include/bytearrayobject.h
* cpython/Objects/clinic/bytearrayobject.c.h

# memory layout

The **ob_alloc** field represents the real allocated size in bytes

**ob_bytes** is the physical begin address, and **ob_start** is the logical begin address

**ob_exports** means how many other objects are sharing this buffer, like reference count in a way

![memory layout](https://img-blog.csdnimg.cn/20190315152551189.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)


# example

## empty bytearray

```python3
>>> a = bytearray(b"")
>>> id(a)
4353755656
>>> b = bytearray(b"")
>>> id(b) # they are not shared
4353755712


```

![empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/empty.png)

## append

after append a charracter 'a', **ob_alloc** becomes 2, **ob_bytes** and **ob_start** all points to same address

```python3
a.append(ord('a'))

```

![append_a](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/append_a.png)

## resize

The size grow pattern is shown in the code

```python3
    /* Need growing, decide on a strategy */
    if (size <= alloc * 1.125) {
        /* Moderate upsize; overallocate similar to list_resize() */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize; resize up to exact size */
        alloc = size + 1;
    }

```

In appending, ob_alloc is 2, and request size is 2, 2 <= 2 * 1.125, so the new allocated size is 2 + (2 >> 3) + 3 ==> 5

```python3
a.append(ord('b'))

```

![resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/resize.png)

## slice

```python3
b = bytearray(b"abcdefghijk")

```

![slice](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/slice.png)

After the slice operation, **ob_start** points to the real beginning of the content, and **ob_bytes** still points to the begin address of the malloced block

```python3
b[0:5] = [1,2]

```

![after_slice](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/after_slice.png)

as long as the slice operation is going to shrink the bytearray, and the **new_size < allocate / 2** is False, the resize operation won't shrink the real malloced size

```python3
b[2:6] = [3, 4]

```

![after2_slice](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/after2_slice.png)

now, in the shrink operation, the **new_size < allocate / 2** is True, the resize operation will be triggered

```python3
b[0:3] = [7,8]

```

![after3_slice](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/after3_slice.png)

The growing pattern in slice operation is the same as the append operation

request size is 6, 6 < 6 * 1.125, so new allocated size is 6 + (6 >> 3) + 3 ==> 9

```python3
b[0:3] = [1,2,3,4]

```

![after_grow_slice](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/after_grow_slice.png)

## ob_exports

what's field **ob_exports** mean ? If you need detail, you can refer to [less-copies-in-python-with-the-buffer-protocol-and-memoryviews](https://eli.thegreenplace.net/2011/11/28/less-copies-in-python-with-the-buffer-protocol-and-memoryviews) and [PEP 3118](https://www.python.org/dev/peps/pep-3118/)

```python3
buf = bytearray(b"abcdefg")

```

![exports](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/exports.png)

the **bytearray** implements the **buffer protocol**, and **memoryview** is able to access the internal data block via the **buffer protocol**, **mybuf** and **buf** are all sharing the same internal block

field **ob_exports** becomes 1, which indicate how many objects currently sharing the internal block via **buffer protocol**

```python3
mybuf = memoryview(buf)
mybuf[1] = 3

```

![exports_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/exports_1.png)

so does **mybuf2** object(**ob_exports** doesn't change because you need to call the c function defined by **buf** object via the **buffer protocol**, **mybuf2** barely calls the slice function of **mybuf**)

```python3
mybuf2 = mybuf[:4]
mybuf2[0] = 1

```

![exports_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/exports_2.png)

**ob_exports** becomes 2

```python3
mybuf3 = memoryview(buf)

```

![exports_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/exports_3.png)

**ob_exports** becomes 0

```python3
del mybuf
del mybuf2
del mybuf3

```

![exports_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/bytearray/exports_4.png)
