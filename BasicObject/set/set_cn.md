# set

#### 相关位置文件
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

#### 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### 内建方法

##### new
* 调用栈
	* static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
		* static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

* 图像表示

![make new set](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/set/make_new_set.png)

##### add
* 调用栈
	* static PyObject *set_add(PySetObject *so, PyObject *key)
		* static int set_add_key(PySetObject *so, PyObject *key)
			* static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)

* 图像表示

