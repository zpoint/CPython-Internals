# set

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#命令行支持及示例)
	* [new](#new)
	* [add](#add)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES?)
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

* **graph representation**

![make new set](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/make_new_set.png)

* ##### **add**
    * call stack
        * static PyObject *set_add(PySetObject *so, PyObject *key)
		    * static int set_add_key(PySetObject *so, PyObject *key)
			    * static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)

* graph representation


      s = set()

![set_empty](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_empty.png)


    s.add(0) # hash(0) & mask == 0

![set_add_0](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_0.png)

    s.add(5) # hash(5) & mask == 0

![set_add_5](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_5.png)

    s.add(16) # hash(16) & mask == 0

![set_add_16](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_16.png)

    s.add(32) # hash(32) & mask == 0

![set_add_32](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_32.png)

    s.add(2) # hash(2) & mask == 0

![set_add_2](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_2.png)

    /*
      now, fill == 5, mask == 7, fill*5 !< mask * 3, need to resize the hash table
      from cpython/Objects/setobject.c
    */
        if ((size_t)so->fill*5 < mask*3)
        return 0;
    return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

![set_add_2_resize](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_2_resize.png)

* ##### **why LINEAR_PROBES?**
    * improve cache locality
    * reduces the cost of hash collisions

* ##### **clear**
    * call stack
        * static PyObject *set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
		    * static int set_clear_internal(PySetObject *so)
				* static void set_empty_to_minsize(PySetObject *so)

* graph representation

      s.clear()

![set_clear](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_clear.png)