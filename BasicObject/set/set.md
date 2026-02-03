# set

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
    * [new](#new)
    * [add](#add)
        * [hash collision](#hash-collision)
        * [resize](#resize)
        * [why LINEAR_PROBES?](#why-LINEAR_PROBES)
    * [clear](#clear)

# related file
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

# memory layout

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# method

* ## **new**
    * call stack
        * static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
            * static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

The following picture shows the value of each field in an empty set

![make new set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/make_new_set.png)

* ## **add**
    * call stack
        * static PyObject *set_add(PySetObject *so, PyObject *key)
            * static int set_add_key(PySetObject *so, PyObject *key)
                * static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)


Initialize a new empty set. Whenever an empty set is created, the **setentry** field points to the **smalltable** field inside the same PySetObject. The default value of **PySet_MINSIZE** is 8, which means if there are fewer than 8 objects in PySetObject, they are stored in the **smalltable**. If there are more than 8 objects, the resize operation will make **setentry** point to a new malloced block.
(Actually, the first time a resize operation takes place, the smalltable won't be filled. There is a factor that triggers the resize operation; please keep reading)

```python3
  s = set()

```

![set_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_empty.png)

The value in the **mask** field is the size of malloced entries minus 1; currently it's 7

```python3
s.add(0) # hash(0) & mask == 0

```

![set_add_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_0.png)

Add a value 5

```python3
s.add(5) # hash(5) & mask == 0

```

![set_add_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_5.png)

* ### hash collision

Add a value 16. Because the mask is 7, hash(16) & 7 ===> 0
CPython uses **LINEAR_PROBES** to solve hash collisions instead of **CHAIN** (linked list)

```c
// pseudocode
#define LINEAR_PROBES 9
while (1)
{
    // i is the hash result
    // find the position according to the hash value
    if (entry in i is empty)
        return entry[i]
    // reach here, means entry[i] already taken
    if (i + LINEAR_PROBES <= mask) {
        for (j = 0 ; j < LINEAR_PROBES ; j++) {
            if (entry in j is empty)
                return j
        }
    }
    // reach here, means entry[i] - entry[i + LINEAR_PROBES] all are taken
    // now, rehash take place
    perturb >>= PERTURB_SHIFT;
    i = (i * 5 + 1 + perturb) & mask;
}

```

The 0th position has been taken, so **LINEAR_PROBES** takes the next empty position, which is the 1st position

```python3
s.add(16) # hash(16) & mask == 0

```

![set_add_16](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_16.png)

The 0th position has been taken. The first time, **LINEAR_PROBES** finds the 1st position, which has also been taken. The second time, **LINEAR_PROBES** finds the 2nd position, which is empty.

```python3
s.add(32) # hash(32) & mask == 0

```

![set_add_32](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_32.png)

* ## **resize**

Let's insert one more item with value 64, still repeating the **LINEAR_PROBES** process, inserting at index 3

```python3
s.add(64) # hash(64) & mask == 0

```

![set_add_64](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64.png)

Now the value in field **fill** is 5 and **mask** is 7, which will trigger the resize operation.
The trigger condition is **fill*5 !< mask * 3**

```python3
// from cpython/Objects/setobject.c
if ((size_t)so->fill*5 < mask*3)
    return 0;
return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

```

After resize, values 32 and 64 still repeat the **LINEAR_PROBES** process, inserted at index 1 and index 2. Because the value in the **mask** field becomes 31 and hash(16) is less than the mask, 16 can be inserted at index 16.
The field **table** no longer points to **smalltable**; instead, it points to a new malloced block

![set_add_64_resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64_resize.png)

* ## **why LINEAR_PROBES**
    * improve cache locality
    * reduces the cost of hash collisions

* ## **clear**
    * call stack
        * static PyObject *set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
            * static int set_clear_internal(PySetObject *so)
                * static void set_empty_to_minsize(PySetObject *so)

Call clear to restore the initial state

```python3
  s.clear()

```

![set_clear](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_clear.png)