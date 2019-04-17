# list

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
	* [new](#new)
	* [append](#append)
	* [pop](#pop)
	* [delete](#delete)
		* [why free list](#why-free-list)

#### related file
* cpython/Objects/listobject.c
* cpython/Objects/clinic/listobject.c.h
* cpython/Include/listobject.h

#### memory layout

![memory layout](https://img-blog.csdnimg.cn/20190214101628263.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### method

* ##### **new**
    * call stack
	    * PyObject * PyList_New(Py_ssize_t size)

* **graph representation**


      l = list()

![list_empty](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/list_empty.png)

* ##### **append**
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
					 /* The growth pattern is:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...
					 * Note: new_allocated won't overflow because the largest possible value
					 * is PY_SSIZE_T_MAX * (9 / 8) + 6 which always fits in a size_t.
					 */
					new_allocated = (size_t)newsize + (newsize >> 3) + (newsize < 9 ? 3 : 6);
					if (new_allocated > (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
					    PyErr_NoMemory();
					    return -1;
					}


* graph representation


    l.append("a") # list_resize, newsize is 1, 1 + (1 >> 3) + 3 = 4

![append_a](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/append_a.png)

    l.append("b")
    l.append("c")
    l.append("d")

![append_d](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/append_d.png)

    l.append("e") # list_resize, newsize is 5, 5 + (5 >> 3) + 3 = 8

![append_e](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/append_e.png)

* ##### **pop**
    * call stack
        * static PyObject *list_pop_impl(PyListObject *self, Py_ssize_t index)


				l.pop()
				# e
				# now, newsize == 4, allocated == 8, allocated >= (newsize >> 1) is False, so resize do nothing

![pop_e](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/pop_e.png)

    l.pop()
    # d
    # now, newsize == 3, allocated == 8, allocated >= (newsize >> 1) is True, new_size = 3 + (3 >> 3) + 3 = 6

![pop_d](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/pop_d.png)

* ##### **clear**
    * call stack
        * static PyObject * list_clear_impl(PyListObject *self)
        	* static int _list_clear(PyListObject *a)


    	l.clear()

![clear](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/clear.png)

* ##### **delete**
    * call stack
        * static void list_dealloc(PyListObject *op)



				#I've altered the source code to print free_list state
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
				#we can see that cpython virtual machine use list object internally, it will interfere with out test result
				a = [[] for i in range(10)]
				print(id(a)) # oops, it's the list we just del
				4481833160
				

![print_id](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/print_id.png)

![del_l](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/del_l.png)

![new_a](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/list/new_a.png)

* ##### **why free list**
    * improve performance
    * reduce memory fragmentation
