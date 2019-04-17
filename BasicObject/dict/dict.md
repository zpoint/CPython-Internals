# dict

### category

Because the **PyDictObject** is a little bit more compilated than other basic object, I will not show every step of _\_setitem_\_/_\_getitem_\_ step by step, instead, I will illustrate in the middle of some concept

* [related file](#related-file)
* [memory layout](#memory-layout)
	* [combined table && split table](#combined-table-&&-split-table)

#### related file
* cpython/Objects/dictobject.c
* cpython/Objects/clinic/dictobject.c.h
* cpython/Include/dictobject.h
* cpython/Include/cpython/dictobject.h


#### memory layout

before we dive into the memory layout of python dictionary, let's imagine what normal dictionary object look like

usually, we implement dictionary as hash table, it takes O(1) time to look up an element, that's what exactly cpython does.

this is an entry, which points to a hash table inside the python dictionary object before python3.6

![entry_before](https://img-blog.csdnimg.cn/20190311111041784.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

If you have many large sparse hash table, it will waste lots of memory. In order to represent the hash table in a more compact way, we split indices and real key value object inside the hashtable(After python3.6)

![entry_after](https://img-blog.csdnimg.cn/20190311114021201.png)

It takes about half of the origin memory to store the same hash table, and we can traverse the hash table in the same order as we insert/delete items. Before python3.6 it's not possible to retain the order of key/value in hash table due to the resize/rehash operation. For those who needs more detail, please refer to [python-dev](https://mail.python.org/pipermail/python-dev/2012-December/123028.html) and [pypy-blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html)

Now, let's see the memory layout

![memory layout](https://img-blog.csdnimg.cn/20190308144931301.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

##### combined table && split table

I'm confused when I first look at the defination of the **PyDictObject**, what's **ma_values** ? why **PyDictKeysObject** looks different from the indices/entries structure above?

the source code says

    /*
    The DictObject can be in one of two forms.

    Either:
      A combined table:
        ma_values == NULL, dk_refcnt == 1.
        Values are stored in the me_value field of the PyDictKeysObject.
    Or:
      A split table:
        ma_values != NULL, dk_refcnt >= 1
        Values are stored in the ma_values array.
        Only string (unicode) keys are allowed.
        All dicts sharing same key must have same insertion order.
    */

In what situation will different dict object shares same keys but different values, only contain unicode keys and no dummy keys(no deleted object), and preserve same insertion order? Let's try.

    # I've altered the source code to print some state information

    class B(object):
    	b = 1

    b1 = B()
    b2 = B()

	# the __dict__ object isn't generated yet
    >>> b1.b
    1
    >>> b2.b
    1

	# __dict__ obect appears, b1.__dict__ and b2.__dict__ are all split table, they shares the same PyDictKeysObject
    >>> b1.b = 3
    in lookdict_split, address of PyDictObject: 0x10bc0eb40, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    >>> b2.b = 4
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    >>> b1.b
    in lookdict_split, address of PyDictObject: 0x10bc0eb40, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    3
    >>> b2.b
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: b
    4
    # after delete a key from split table, it becomes combined table
    >>> b2.x = 3
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: x
    >>> del b2.x
    in lookdict_split, address of PyDictObject: 0x10bdbbc00, address of PyDictKeysObject: 0x10bd8cca8, key_str: x
    # now, no more lookdict_split
	>>> b2.b
	4

The split table implementation can save a lots of memory if you have many instances of same class. For more detail, please refer to [PEP 412 -- Key-Sharing Dictionary](https://www.python.org/dev/peps/pep-0412/)


#### indices and entries

Let's analyze some source code to understand how indices/entries implement in **PyDictKeysObject**, what **char dk_indices[]** means in **PyDictKeysObject**?

    /*
    dk_indices is actual hashtable.  It holds index in entries, or DKIX_EMPTY(-1)
    or DKIX_DUMMY(-2).
    Size of indices is dk_size.  Type of each index in indices is vary on dk_size:

    * int8  for          dk_size <= 128
    * int16 for 256   <= dk_size <= 2**15
    * int32 for 2**16 <= dk_size <= 2**31
    * int64 for 2**32 <= dk_size

    dk_entries is array of PyDictKeyEntry.  It's size is USABLE_FRACTION(dk_size).
    DK_ENTRIES(dk) can be used to get pointer to entries.

    NOTE: Since negative value is used for DKIX_EMPTY and DKIX_DUMMY, type of
    dk_indices entry is signed integer and int16 is used for table which
    dk_size == 256.
    */

    #define DK_SIZE(dk) ((dk)->dk_size)
    #if SIZEOF_VOID_P > 4
    #define DK_IXSIZE(dk)                          \
        (DK_SIZE(dk) <= 0xff ?                     \
            1 : DK_SIZE(dk) <= 0xffff ?            \
                2 : DK_SIZE(dk) <= 0xffffffff ?    \
                    4 : sizeof(int64_t))
    #else
    #define DK_IXSIZE(dk)                          \
        (DK_SIZE(dk) <= 0xff ?                     \
            1 : DK_SIZE(dk) <= 0xffff ?            \
                2 : sizeof(int32_t))
    #endif
    #define DK_ENTRIES(dk) \
        ((PyDictKeyEntry*)(&((int8_t*)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]))
