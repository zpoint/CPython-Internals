# Set

#### related file
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### method

##### new
* call stack
	* static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
		* static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

* graph representation

![make new set](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/make_new_set.png)

##### add
* call stack
	* static PyObject *set_add(PySetObject *so, PyObject *key)
		* static int set_add_key(PySetObject *so, PyObject *key)
			* static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)

* graph representation

