# dict

### contents

Because the **PyDictObject** is a little bit more complicated than other basic object, I will not show _\_setitem_\_/_\_getitem_\_ step by step, instead, I will illustrate in the middle of some concept

* [related file](#related-file)
* [memory layout](#memory-layout)
	* [combined table && split table](#combined-table-&&-split-table)
	* [indices and entries](#indices-and-entries)
* [hash collisions and delete](#hash-collisions-and-delete)
* [resize](#resize)
* [variable size indices](#variable-size-indices)
* [free list](#free-list)

* [delete](#delete)
	* [why mark as DKIX_DUMMY](#why-mark-as-DKIX_DUMMY)
	* [delete in entries](#delete-in-entries)
* [end](#end)

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

![dict_shares](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict_shares.png)

#### indices and entries

Let's analyze some source code to understand how indices/entries implement in **PyDictKeysObject**, what **char dk_indices[]** means in **PyDictKeysObject**?
(It took me sometimes to figure out)

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

Let's rewrite the marco

    #define DK_ENTRIES(dk) \
        ((PyDictKeyEntry*)(&((int8_t*)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]))

to

    // assume int8_t can fit into the indices array
    size_t indices_offset = DK_SIZE(dk) * DK_IXSIZE(dk);
    int8_t *pointer_to_indices = (int8_t *)(dk->dk_indices);
    int8_t *pointer_to_entries = pointer_to_indices + indices_offset;
    PyDictKeyEntry *entries = (PyDictKeyEntry *) pointer_to_entries;

now, the overview is clear

![dictkeys_basic](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dictkeys_basic.png)

#### hash collisions and delete

how pyrthon handle hash collisions in dict object? Instead of depending on a good hash function, python uses "perturb" strategy, let's read some source code and have a try


	j = ((5*j) + 1) mod 2**i
    0 -> 1 -> 6 -> 7 -> 4 -> 5 -> 2 -> 3 -> 0 [and here it's repeating]
    perturb >>= PERTURB_SHIFT;
    j = (5*j) + 1 + perturb;
    use j % 2**i as the next table index;

I've altered the source code to print some information


    >>> d = dict()
    >>> d[1] = 1
    : 1, ix: -1, address of ep0: 0x10870d798, dk->dk_indices: 0x10870d790
    ma_used: 0, ma_version_tag: 11313, PyDictKeyObject.dk_refcnt: 1, PyDictKeyObject.dk_size: 8, PyDictKeyObject.dk_usable: 5, PyDictKeyObject.dk_nentries: 0
    DK_SIZE(dk): 8, DK_IXSIZE(dk): 1, DK_SIZE(dk) * DK_IXSIZE(dk): 8, &((int8_t *)((dk)->dk_indices))[DK_SIZE(dk) * DK_IXSIZE(dk)]): 0x10870d798

	>>> repr(d)
    ma_used: 1, ma_version_tag: 11322, PyDictKeyObject.dk_refcnt: 1, PyDictKeyObject.dk_size: 8, PyDictKeyObject.dk_usable: 4, PyDictKeyObject.dk_nentries: 1
    index: 0 ix: -1 DKIX_EMPTY
    index: 1 ix: 0 me_hash: 1, me_key: 1, me_value: 1
    index: 2 ix: -1 DKIX_EMPTY
    index: 3 ix: -1 DKIX_EMPTY
    index: 4 ix: -1 DKIX_EMPTY
    index: 5 ix: -1 DKIX_EMPTY
    index: 6 ix: -1 DKIX_EMPTY
    index: 7 ix: -1 DKIX_EMPTY
	'{1: 1}'


![hh_1](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_1.png)

    d[4] = 4

![hh_2](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_2.png)

    d[7] = 111

![hh_3](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_3.png)

	# delete, mark as DKIX_DUMMY
    # notice, dk_usable and dk_nentries don't change
    del d[4]

![hh_4](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_4.png)

	# notice, dk_usable and dk_nentries now change
	d[0] = 0

![hh_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_5.png)

	d[16] = 16
    # hash (16) & mask == 0
    # but position 0 already taken by key: 0, value: 0
    # currently perturb = 16, PERTURB_SHIFT = 5, i = 0
    # so, perturb >>= PERTURB_SHIFT ===> perturb == 0
    # i = (i*5 + perturb + 1) & mask ===> i = 1
    # now, position 1 already taken by key: 1, value: 1
    # currently perturb = 0, PERTURB_SHIFT = 5, i = 1
    # so, perturb >>= PERTURB_SHIFT ===> perturb == 0
    # i = (i*5 + perturb + 1) & mask ===> i = 6
    # position 6 is empty, so we take it

![hh_6](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/hh_6.png)

#### resize

	# now, dk_usable is 0, dk_nentries is 5
    # let's insert one more item
    d[5] = 5
    # step1: resize, when resizing, the deleted object which mark as DKIX_DUMMY in entries won't be copyed

![resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/resize.png)

	# step2: insert key: 5, value: 5


#### variable size indices
Notice, the indices array is variable size. when size of your hash table is <= 128, type of each item is int_8, int16 and int64 for bigger table. The variable size indices array can save memory usage.

#### free list

	static PyDictObject *free_list[PyDict_MAXFREELIST];

cpython also use free_list to reuse the deleted hash table, to avoid memory fragment and improve performance, I've illustrated free_list in [list object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list.md#why-free-list)

#### delete

the [lazy deletion](https://en.wikipedia.org/wiki/Lazy_deletion) strategy is used for deletiion in dict object

##### why mark as DKIX_DUMMY

why mark as **DKIX_DUMMY** in the indices?

there're totally three kinds of value for each slot in indices, **DKIX_EMPTY**(-1), **DKIX_DUMMY**(-2) and **Active/Pending**(>=0), if you mark as  **DKIX_EMPTY** instead of **DKIX_DUMMY**, the "perturb" strategy for inserting and finding key/values will fail. Imagine you have a hash table in size 8, this is the indices in hash table

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [3]  [4] [DKIX_EMPTY]
    index:    0    1         2      3        4           5          6    7     8

when you search for a item with hash value 0, and real position is in entries[4]

this is the searching process according to the "perturb" strategy

	0 -> 1 -> 6 -> 7(found, no need to go on)

if you delete the 6th indices, and mark it as **DKIX_EMPTY**, when you search for the same item again

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [DKIX_EMPTY]  [4] [DKIX_EMPTY]
    index:    0    1         2      3        4           5               6        7        8

the searching process now becomes

	0 -> 1 -> 6(it's DKIX_EMPTY, the last inserted item with same hash value must be inserted in indices[1], since no matched result, the item being searched is not in this table)

the item being searched is in indices[7], but "perturb" strategy stopped before indices[7] due to the wrong **DKIX_EMPTY** in indices[6], if we mark deleted as **DKIX_DUMMY**

	indices: [0]  [1] [DKIX_EMPTY] [2] [DKIX_EMPTY] [DKIX_EMPTY]   [DKIX_DUMMY]  [4] [DKIX_EMPTY]
    index:    0    1         2      3        4           5               6        7        8

the searching process becomes

	0 -> 1 -> 6(DKIX_DUMMY, inserted before, but deleted, keep searching) -> 7(found, no need to go on)

also, the indices with **DKIX_DUMMY** can be inserted for new item

##### delete in entries

dict object need to guarantee the [inserted order](https://mail.python.org/pipermail/python-dev/2017-December/151283.html), the delete operation can't shuffle objects in **entries**

leave the entries[i] to empry, and packing these empry entries later in the resize operation can keep the time complexity of delete operation in amortize O(1)

deleting entries from dictionaries is not very common, in some case a little bit slower is acceptable([PyPy Status Blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html))

#### end

now, you understand how python dictionary object work internally.