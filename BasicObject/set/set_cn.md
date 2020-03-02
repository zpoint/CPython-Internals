# set

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [内建方法](#内建方法)
	* [new](#new)
	* [add](#add)
		* [哈希碰撞](#哈希碰撞)
		* [resize](#resize)
	    * [为什么采用-LINEAR_PROBES?](#为什么采用-LINEAR_PROBES)
	* [clear](#clear)

# 相关位置文件
* cpython/Objects/setobject.c
* cpython/Include/setobject.h

# 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190312123042232.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

# 内建方法

* ## **new**
    * 调用栈
	    * static PyObject * set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
		    * static PyObject * make_new_set(PyTypeObject *type, PyObject *iterable)

一个空的字符串各个字段的值如下所示

![make new set](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/make_new_set.png)

* ## **add**
    * 调用栈
	    * static PyObject *set_add(PySetObject *so, PyObject *key)
		    * static int set_add_key(PySetObject *so, PyObject *key)
			    * static int set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)


初始化一个空的集合对象，每当你创建一个空的集合对象时，**setentry** 这个字段都会指向 **smalltable**, 这两个字段都在 **PySetObject** 这同一个内存块里. **PySet_MINSIZE** 的默认大小是 8, 意思是如果你的集合对象存储了不到 8 个 python object 时，这些对象会存储到 **smalltable** 里，当超过 8 个对象存储时候，resize 会重新分配哈希表的空间，并把 **setentry** 指向新分配的空间
(事实上，set 对象并不会等到把这8个坑位全部填满才触发resize, 达到了阈值的判定公式就会触发 resize, 请往下看)

```python3
  s = set()

```

![set_empty](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_empty.png)

**mask** 字段里面的值是真正分配的 setentry 所拥有的内存空间的大小 - 1, 当前他的值是 7

```python3
s.add(0) # hash(0) & mask == 0

```

![set_add_0](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_0.png)

增加一个值 5

```python3
s.add(5) # hash(5) & mask == 0

```

![set_add_5](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_5.png)

* ### 哈希碰撞

增加一个值 16, 因为 mask 上面的值是 7, hash(16) & 7 ===> 0, 根据哈希值他们会存储到同一个位置
通常我们会用开链法处理哈希，这里 cpython 使用了一个叫 **LINEAR_PROBES** 的方法处理这个问题

```c

// 伪代码
#define LINEAR_PROBES 9
while (1)
{
    // i 是当前的哈希值
    // 根据哈希值找到哈希表对应的位置
    if (entry in i is empty)
    	return entry[i]
    // 到了这里，说明 entry[i] 位置已经被占了
    if (i + LINEAR_PROBES <= mask) {
        for (j = 0 ; j < LINEAR_PROBES ; j++) {
        	if (entry in j is empty)
            	return j
        }
    }
    // 到了这里，说明 entry[i] - entry[i + LINEAR_PROBES] 的位置都被占了
    // 此时，才进行重新哈希
    perturb >>= PERTURB_SHIFT;
    i = (i * 5 + 1 + perturb) & mask;
}

```

第 0 个位置已经被占用了，会从第 0 个位置开始往后最多找 LINEAR_PROBES 个位置，如果找不到再进行重新哈希，并重复前面的过程
刚好从第0个位置开始，第一个位置未被占用，返回

```python3
s.add(16) # hash(16) & mask == 0

```

![set_add_16](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_16.png)

```python3
s.add(32) # hash(32) & mask == 0

```

第 0 个位置已经被占用了，在 **LINEAR_PROBES** 过程中，第一次找到了第一个位置，发现也被占用了，再往后找一个位置，发现第2个位置是空的，所以直接占用他

![set_add_32](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_32.png)

* ## **resize**

我们再插入一个 64, 依然重复上面的 **LINEAR_PROBES** 过程，插入到 3 个位置

```python3
s.add(64) # hash(64) & mask == 0

```

![set_add_64](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64.png)

此时, fill 值为 5, mask 值为 7, 触发 resize 的判定条件是 fill*5 !< mask * 3

```python3

// 来自 cpython/Objects/setobject.c
if ((size_t)so->fill*5 < mask*3)
	return 0;
return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

```

这是 resize 之后的图表，此时 32 和 64 依然按照 **LINEAR_PROBES** 过程插入 1 和 2 的位置，由于 mask 变成了 31，hash(16) 能对应到下标为 15 的位置上
并且 table 指向了新申请的空间

![set_add_64_resize](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_add_64_resize.png)

* ## **为什么采用 LINEAR_PROBES**
    * 更好的利用 cache locality
    * 减小哈希碰撞，避免链式存储出现很长的链表

* ## **clear**
    * 调用栈
        * static PyObject *set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
		    * static int set_clear_internal(PySetObject *so)
				* static void set_empty_to_minsize(PySetObject *so)

调用 clear, 恢复初始状态

```python3
  s.clear()

```

![set_clear](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/set/set_clear.png)