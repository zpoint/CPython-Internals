# list

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [append](#append)
* [pop](#pop)
* [sort](#sort)
* [free_list](#free_list)

# related file
* cpython/Objects/listobject.c
* cpython/Objects/clinic/listobject.c.h
* cpython/Include/listobject.h

# memory layout

![memory layout](https://img-blog.csdnimg.cn/20190214101628263.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# append

the basic object's name is list, but the actual implementation is more like a `vector` in `C++`

let's initialize an empty `list`

	l = list()

field `ob_size` stores the actual size, it's type is `Py_ssize_t` which is usually 64 bit, 1 << 64 is a very large number, usually you will run out of RAM before overflow the `ob_size` field

![list_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_empty.png)

and append some elements into the `list`

	l.append("a")

`ob_size` becomes 1, and `ob_item` will point to a newly malloced block with size 4

![append_a](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_a.png)

	l.append("b")
    l.append("c")
    l.append("d")

now the `list` is full

![append_d](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_d.png)

if we append one more element

	l.append("e")

this is the resize pattern

	/* cpython/Objects/listobject.c */
    /* currently: new_allocated = 5 + (5 >> 3) + 3 = 8 */
	new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);

![append_e](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_e.png)

you see, it's really more like a `C++ vector`

# pop

the append operation will only trigger the `list`'s resize when the `list` is full

what about pop ?

    >>> l.pop()
    'e'

![pop_e](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/pop_e.png)

    >>> l.pop()
    'd'

the realloc is called and the malloced size is shrinked, actually the resize function will be called each time you call `pop`

but the actual realloc will be called only if the newsize falls lower than half the allocated size

    /* cpython/Objects/listobject.c */
    /* allocated: 8, newsize: 3, 8 >= 3 && (3 >= 4?), no */
    if (allocated >= newsize && newsize >= (allocated >> 1)) {
        assert(self->ob_item != NULL || newsize == 0);
        Py_SIZE(self) = newsize;
        return 0;
    }
    /* ... */
    /* 3 + (3 >> 3) + 3 = 6 */
    new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);


![pop_d](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/pop_d.png)

# sort

