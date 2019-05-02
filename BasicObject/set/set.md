# set

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
	* [new](#new)
	* [add](#add)
		* [hash collision](#hash-collision)
		* [resize](#resize)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES)
	* [clear](#clear)

#### related file
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### method

* ##### **new**
    * call stack
	    * static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
		    * static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

the following picture shows value in each field in an empty set

![make new set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/make_new_set.png)

* ##### **add**
    * call stack
        * static PyObject *set_add(PySetObject *so, PyObject *key)
		    * static int set_add_key(PySetObject *so, PyObject *key)
			    * static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)


initialize a new empty set, whenever an empty set created, the **setentry** field points to **smalltable** field inside the same PySetObject, default value of **PySet_MINSIZE** is 8, it means if there are less than 8 objects in PySetObject, they are stored in the **smalltable**, if there are more than 8 objects, the resize operation will make **setentry** points to a new malloced block
(Actually, the first time resize operation takes place, the smalltable won't be filled, there is a factor to trigger the resize operation, please keep reading)

      s = set()

![set_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_empty.png)

the value in **mask** field is the size of malloced entries - 1, currently it's 7

    s.add(0) # hash(0) & mask == 0

![set_add_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_0.png)

add a value 5

    s.add(5) # hash(5) & mask == 0

![set_add_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_5.png)

* ###### hash collision

add a value 16, because the mask is 7, hash(16) & 7 ===> 0
cpython use **LINEAR_PROBES** to solve hash collision instead of **CHAIN**(linked list)

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

the 0th position has been taken, so the **LINEAR_PROBES** take the next empty position, which is 1th position

    s.add(16) # hash(16) & mask == 0

![set_add_16](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_16.png)

the 0th position has been taken, the first time **LINEAR_PROBES** find the 1th position, which also has been taken, the second time **LINEAR_PROBES** find the 2th position which is empty.

    s.add(32) # hash(32) & mask == 0

![set_add_32](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_32.png)

* ##### **resize**

let's insert one more item with value 64, still repeat the **LINEAR_PROBES** progress, insert to position at index 3

    s.add(64) # hash(64) & mask == 0

![set_add_64](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64.png)

now, value in field **fill** is 5, **mask** is 7, it will trigger the resize operation
the trigger condition is **fill*5 !< mask * 3**

	// from cpython/Objects/setobject.c
	if ((size_t)so->fill*5 < mask*3)
		return 0;
	return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

after resize，value 32 and 64 still repeat the **LINEAR_PROBES** progress, inserted at index 1 and index 2, because the value in **mask** field becoms 31, hash(16) is less than mask, so 16 can be inserted to index 15
And field **table** no longer points to **smalltable**, instead, it points to a new malloced block

![set_add_64_resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64_resize.png)

* ##### **why LINEAR_PROBES**
    * improve cache locality
    * reduces the cost of hash collisions

* ##### **clear**
    * call stack
        * static PyObject *set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
		    * static int set_clear_internal(PySetObject *so)
				* static void set_empty_to_minsize(PySetObject *so)

call clear to restore the initial state

      s.clear()

![set_clear](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_clear.png)