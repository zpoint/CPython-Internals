# list

# category

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [内建方法](#内建方法)
	* [new](#new)
	* [append](#append)
	* [pop](#pop)
	* [delete 和 free list](#delete-和-free-list)
		* [为什么用 free list](#为什么用-free-list)

# 相关位置文件
* cpython/Objects/listobject.c
* cpython/Objects/clinic/listobject.c.h
* cpython/Include/listobject.h

# 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190214101628263.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# 内建方法

* ## **new**
    * call stack
	    * PyObject * PyList_New(Py_ssize_t size)

* **图像表示**


      l = list()

![list_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/list_empty.png)

* ## **append**
    * call stack
        * static PyObject *list_append(PyListObject *self, PyObject *object)
		    * static int app1(PyListObject *self, PyObject *v)


					#define PyList_SET_ITEM(op, i, v) (((PyListObject *)(op))->ob_item[i] = (v))

					static int app1(PyListObject *self, PyObject *v)
					{
					    Py_ssize_t n = PyList_GET_SIZE(self);

					    assert (v != NULL);
					    if (n == PY_SSIZE_T_MAX) {
						PyErr_SetString(PyExc_OverflowError,
						    "cannot add more objects to list");
						return -1;
					    }

					    if (list_resize(self, n+1) < 0)
						return -1;

					    Py_INCREF(v);
					    PyList_SET_ITEM(self, n, v);
					    return 0;
					}

					/* list_resize */
					 /* list 大小的增长符合如下规律:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...
					 * 即使再大也不会溢出，因为最大就是这么大 PY_SSIZE_T_MAX * (9 / 8) + 6
					 * 比 size_t 小
					 */
					new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);
					if (new_allocated > (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
					    PyErr_NoMemory();
					    return -1;
					}


* 图像示例


      l.append("a") # 调用 list_resize, 此时 newsize == 1, 1 + (1 >> 3) + 3 = 4

![append_a](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_a.png)

    l.append("b")
    l.append("c")
    l.append("d")

![append_d](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_d.png)

    l.append("e") # 调用 list_resize,此时  newsize == 5, 5 + (5 >> 3) + 3 = 8

![append_e](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/append_e.png)

* ## **pop**
    * call stack
        * static PyObject *list_pop_impl(PyListObject *self, Py_ssize_t index)


				l.pop()
				# e
				# 此时, newsize == 4, allocated == 8, allocated >= (newsize >> 1) 是不成立的, 所以 resize 直接返回，不需要裁剪大小


![pop_e](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/pop_e.png)

    l.pop()
    # d
    # 此时, newsize == 3, allocated == 8, allocated >= (newsize >> 1) 是成立的, new_size = 3 + (3 >> 3) + 3 = 6

![pop_d](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/pop_d.png)

* ## **clear**
    * call stack
        * static PyObject * list_clear_impl(PyListObject *self)
        	* static int _list_clear(PyListObject *a)


    	l.clear()

![clear](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/clear.png)

* ## **delete 和 free list**
    * call stack
        * static void list_dealloc(PyListObject *op)


				#我修改了部分源代码，打印出 free_list 的状态
				print(id(l)) # 4481833160
				>>> id(l)
				PyList_New, 0, numfree: 5
				PyList_New, 0, numfree: 4
				PyList_New, 0, numfree: 3
				PyList_New, 0, numfree: 2
				PyList_New, 2, numfree: 1
				PyList_New, 2, numfree: 1
				PyList_New, 1, numfree: 1
				PyList_New, 0, numfree: 5
				4320315592
				PyList_New, 0, numfree: 6
				>>> del l
				PyList_New, 0, numfree: 5
				PyList_New, 0, numfree: 4
				PyList_New, 0, numfree: 3
				PyList_New, 0, numfree: 2
				PyList_New, 1, numfree: 1
				PyList_New, 1, numfree: 1
				PyList_New, 1, numfree: 1
				PyList_New, 0, numfree: 7
				#我们发现 python 虚拟机自己执行命令的时候也用了很多 list 对象，可能影响到我们对 free_list 的观察
				a = [[] for i in range(10)]
				print(id(a)) # 这个id就是我们上面删除的 l 的 id
				4481833160

![print_id](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/print_id.png)

![del_l](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/del_l.png)

![new_a](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/list/new_a.png)

* ## **为什么用 free list**
    * 提高性能，malloc 这个系统调用是很耗时的(python自己也实现了一套内存管理)
    * 减小内存碎片
