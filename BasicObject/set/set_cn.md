# set

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [内建方法](#内建方法)
	* [new](#new)
	* [add](#add)
	    * [为什么采用-LINEAR_PROBES?](#为什么采用-LINEAR_PROBES?)
	* [clear](#clear)

#### 相关位置文件
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

#### 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### 内建方法

* ##### **new**
    * 调用栈
	    * static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
		    * static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

* 图像表示

![make new set](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/make_new_set.png)

* ##### **add**
    * 调用栈
	    * static PyObject *set_add(PySetObject *so, PyObject *key)
		    * static int set_add_key(PySetObject *so, PyObject *key)
			    * static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)

* 图像表示


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
      现在, fill 值为 5, mask 值为 7, fill*5 !< mask * 3, 需要为哈希表重新分配空间
      来自 cpython/Objects/setobject.c
    */
        if ((size_t)so->fill*5 < mask*3)
        return 0;
    return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

![set_add_2_resize](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_add_2_resize.png)

* ##### **为什么采用 LINEAR_PROBES?**
    * 更好的利用 cache locality
    * 减小哈希碰撞，避免链式存储出现很长的链表

* ##### **clear**
    * 调用栈
        * static PyObject *set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
		    * static int set_clear_internal(PySetObject *so)
				* static void set_empty_to_minsize(PySetObject *so)

* graph representation

      s.clear()

![set_clear](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/set_clear.png)