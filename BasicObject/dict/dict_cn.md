# dict

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
* cpython/Objects/dictobject.c
* cpython/Objects/clinic/dictobject.c.h
* cpython/Include/dictobject.h


#### memory layout

before we dive into the memory layout of python dictionary, let's imagine what normal dictionary object look like

usually, we implement dictionary as hash table, it takes O(1) time to look up an element, that's what exactly cpython does.

this is an entry, which points to a hash table inside the python dictionary object before python3.6

![entry_before](https://img-blog.csdnimg.cn/20190311111041784.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

If you have many large sparse hash table, it will waste lots of memory. In order to represent the hash table in a more compact way, we split indices and real key value object inside the hashtable(After python3.6)

![entry_after](https://img-blog.csdnimg.cn/20190311114021201.png)

now, it takes about half of the origin memory to store the same hash table, and we can traverse the hash table in the same order as we insert/delete items. Before python3.6 it's not possible to retain the order of key/value in hash table due to the resize/rehash poeration. For those who needs more detail, please refer to [python-dev](https://mail.python.org/pipermail/python-dev/2012-December/123028.html) and [pypy-blog](https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html)

Now, let's see the memory layout

![memory layout](https://img-blog.csdnimg.cn/20190308144931301.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

#### method
