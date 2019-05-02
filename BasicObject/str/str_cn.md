# str

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [字符串转换](#字符串转换)
* [interned](#interned)
* [kind](#kind)
	* [unicode底层存储方式总结](#unicode底层存储方式总结)
* [compact](#compact)

#### 相关位置文件
* cpython/Objects/unicodeobject.c
* cpython/Include/unicodeobject.h
* cpython/Include/cpython/unicodeobject.h

#### 内存构造

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/layout.png)

如果你对 c 语言的 bit-fields 有疑问，请参考 [When to use bit-fields in C?](https://stackoverflow.com/questions/24933242/when-to-use-bit-fields-in-c) 和 [“:” (colon) in C struct - what does it mean?](https://stackoverflow.com/questions/8564532/colon-in-c-struct-what-does-it-mean)

#### 字符串转换

在我们深入的看 unicod 对象如何创建，调整空间之前，我们先来看下 **PyUnicode_AsUTF8** 这个 c 函数

    /*
       每当我需要在 cpython 里面尝试把 一个 PyUnicodeObject 转换成 const char* 指针时，我会调用这个函数
       const char *s = PyUnicode_AsUTF8(py_object_to_be_converted)
       我们来看看这个函数的定义
    */
    const char *
	PyUnicode_AsUTF8(PyObject *unicode)
	{
    	return PyUnicode_AsUTF8AndSize(unicode, NULL);
	}

    /* 找到 PyUnicode_AsUTF8AndSize 这个函数 */
    const char *
    PyUnicode_AsUTF8AndSize(PyObject *unicode, Py_ssize_t *psize)
    {
        PyObject *bytes;

		/* 开始之前会做一些边界检查，先忽略这部分检查 */
        /* PyUnicode_UTF8 检查 unicode 对象的 compact flag, 并根据这个 flag 返回一个指向 char 的指针，看下图 */

        if (PyUnicode_UTF8(unicode) == NULL) {
            assert(!PyUnicode_IS_COMPACT_ASCII(unicode));
            /* bytes 的类型是 PyBytesObject，bytes 一共可以有4种不同的可能，详见下图 */
            bytes = _PyUnicode_AsUTF8String(unicode, NULL);
            if (bytes == NULL)
                return NULL;
            /* 把 ((PyCompactUnicodeObject*)(unicode))->utf8 设置为新申请的空间 */
            _PyUnicode_UTF8(unicode) = PyObject_MALLOC(PyBytes_GET_SIZE(bytes) + 1);
            if (_PyUnicode_UTF8(unicode) == NULL) {
            	/* 检查 malloc 是否失败 */
                PyErr_NoMemory();
                Py_DECREF(bytes);
                return NULL;
            }
            /* 把 ((PyCompactUnicodeObject*)(unicode))->utf8_length 设置为 bytes 的长度 */
            _PyUnicode_UTF8_LENGTH(unicode) = PyBytes_GET_SIZE(bytes);
            /* 把编码后的真正的 bytes 复制到 unicode 对象里 */
            memcpy(_PyUnicode_UTF8(unicode),
                      PyBytes_AS_STRING(bytes),
                      _PyUnicode_UTF8_LENGTH(unicode) + 1);
            Py_DECREF(bytes);
        }

        if (psize)
            *psize = PyUnicode_UTF8_LENGTH(unicode);
        return PyUnicode_UTF8(unicode);
    }

PyUnicode_UTF8

![pyunicode_utf8](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/pyunicode_utf8.png)

_PyUnicode_UTF8

![_PyUnicode_UTF8.png](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8.png)

_PyUnicode_AsUTF8String

![_PyUnicode_AsUTF8String](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_AsUTF8String.png)

_PyUnicode_UTF8_LENGTH

![_PyUnicode_UTF8_LENGTH](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8_LENGTH.png)


我们来初始化一个空字符串看看


	# 初始化一个空字符串
	s = ""
    # 注意了，因为 s 的 compact 和 ascii 都为 1，所以字段 utf8_length 的地址作为真正存储字符串值的启始地址

![empty_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/empty_s.png)

	s = "s"
    repr(s) # 我像往常一样更改了源代码，可以打印出更多详细信息
    address of char *utf8: 0x1091d6ea0, content of char *utf8: 0x1
    hash: 24526920963829810, interned: 1, kind: PyUnicode_1BYTE_KIND,
    compact: 1, ready: 1, ascii: 1, ascii length: 1, utf8 length: 115, ready: 0
    address of unicode: 0x1091d6e68, PyUnicode_UTF8(unicode): 0x1091d6e98
    calling PyUnicode_AsUTF8(unicode): s


细心的读者应该注意到了 在 **PyCompactUnicodeObject** 这个对象里面的 **utf8_length** 位置的值是 115, 并且 chr(115) 的值是 's', 这个位置就是标准的以 \0 为结束符的 c 字符串的开始地址

![s_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/s_s.png)


	s = "aaa"

![aaa](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/aaa.png)

#### interned

所有 **interned** 值设置为 1 的 unicode 对象都会被保存在一个叫 **interned** 的全局变量的字典对象里，
所有新创建的同样的 unicode 对象都会指向这个字典已经存在的对象，**interned** 这个全局字典实现了单例模式

    static PyObject *interned = NULL;

对于 **interned** 为 1 的 unicode 对象，如果你释放了这个 **unicode** 对象，并且再初始化一个值相同的对象，他们的id会是相同的，你第一次创建他的时候就存储在了 **interned** 这个全局字典里

	>>> id(s)
    4314134768
    >>> del s
    >>> s = "aaa"
    >>> id(s)
    4314134768
    >>> y = "aaa"
    >>> id(y)
    4314134768


#### kind

**kind** 在 **PyASCIIObject** 里, 这个值表示 unicode 对象里面真正的字节的存储方式，总共有 4 种不同的形式

* PyUnicode_WCHAR_KIND

在 python 层， 我暂时没有找到定义一个 **kind** 值为 **PyUnicode_WCHAR_KIND** 的**unicode** 对象的方法, 也许只能在  c/c++ 层做到

* PyUnicode_1BYTE_KIND
	* 8 bits/character
	* ascii flag set ?
		* ascii flag 值为 1: U+0000-U+007F
		* ascii flag 值为 0: 至少一个字符在 U+0080-U+00FF 里

**utf8_length** 这个地方仍然存储了 \0 终结形式的c字符串，但是 **interned** 这个值并不为 0, 只有在特定字符区间范围内的字符串会存储到 **interned** 全局字典

	s = "\u007F\u0000"
    id(s)
    4519246896
    del s
    s = "\u007F\u0000" # same id as previous one, 即使 interned 为 0, id 也与前一个相同
    4519246896
    del s
    gc.collect() # gc.collect 会把 interned 为 1 的也一并删除
    s = "\u007F\u0000"
    id(s)
    4519245680
    s2 = "\u007F\u0000"
    id(s2)
    4519247152

![1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/1_byte_kind.png)

让我们尝试定义一个包含有 **\u0088** 的字符串

	s = "\u0088\u0011\u00f1"

现在，因为第一个字符为 **U+0088**, **ascii** 的值变为 0,
**PyUnicode_UTF8(unicode)** 不再返回 **utf8_length** 这里的地址，而是返回 **char *utf8** 这个字段里面存储的值，这个值为 0

如果 **PyUnicode_UTF8(unicode)** 为 0，这三个 unicode 字符存在哪里呢，我们还没有用过 **PyUnicodeObject** 里面的 **data** 字段，我们尝试下打印这里面的字段看看
(花了我一点点时间才找到打印 latin1 这个字段里的值的方法)


    static PyObject *
    unicode_repr(PyObject *unicode)
    {
    	...
            /*
            // 有一些官方的宏可以用来直接获取 data 字段里的第 n 个值，比如
            Py_ssize_t isize = PyUnicode_GET_LENGTH(unicode);
            Py_ssize_t idata = PyUnicode_DATA(unicode);
            int ikind = PyUnicode_KIND(unicode);
            Py_UCS4 ch = PyUnicode_READ(ikind, idata, i); // 无论 ikind 是什么值，总用最大的 Py_UCS4 接住
            // 为了使自己更好的理解不同的表示方法，我用自己写的方法去获取
            */
            switch (_PyUnicode_STATE(unicode).kind)
            {
                case (PyUnicode_1BYTE_KIND):
                    printf("PyUnicode_1BYTE_KIND, ");
                    if (PyUnicode_UTF8(unicode) == 0)
                    {

                        char *value = &(((PyUnicodeObject *)unicode)->data);
                        printf("\nPyUnicodeObject->latin1: ");
                        for (size_t i = 0; i < _PyUnicode_LENGTH(unicode); ++i)
                        {
                            printf("%#hhx ", *(value + i));
                        }

                        printf("\n");
                    }
                    break;
                case (PyUnicode_2BYTE_KIND):
                    printf("PyUnicode_2BYTE_KIND, ");
                    break;
                case (PyUnicode_4BYTE_KIND):
                    printf("PyUnicode_4BYTE_KIND, ");
                    break;
                default:
                    printf("unknown kind: %d, ", _PyUnicode_STATE(unicode).kind);
            }
    ...
    }

重新编译后，我们就可以在 repr() 函数里面追踪 latin1 的内容了

	>>> repr(s)
    PyUnicode_1BYTE_KIND,
	PyUnicodeObject->latin1: 0x88 0x11 0xf1

![no_ascii_1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/no_ascii_1_byte_kind.png)

* PyUnicode_2BYTE_KIND
	* 16 bits/character
	* all characters are in range U+0000-U+FFFF
	* at least one character is in the range U+0100-U+FFFF

我们可以用同样的方法去追踪**data**里面的内容，现在这个字段叫做**ucs2**了(**ucs2** 或者 **latin1** 拥有不同的名称，但是他们在同个 union 里，开始地址是相同的，只是长度不同而已)

	>>> s = "\u0011\u0111\u1111"
    >>> repr(s)
    kind: PyUnicode_2BYTE_KIND,  PyUnicodeObject->ucs2: 0x11 0x111 0x1111

现在 **kind** 里的值是 **PyUnicode_2BYTE_KIND** 并且每个字符都需要花费 2个字节去存储

![2_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/2_byte_kind.png)

* PyUnicode_4BYTE_KIND
	* 32 bits/character
	* all characters are in the range U+0000-U+10FFFF
	* at least one character is in the range U+10000-U+10FFFF

现在, kind 字段里存储的值变成了 **PyUnicode_4BYTE_KIND**

	>>> s = "\u00ff\U0010FFFF\U00100111\U0010FFF1"
    >>> repr(s)
    kind: PyUnicode_4BYTE_KIND, PyUnicodeObject->ucs4: 00xff 0x10ffff 0x100111 0x10fff1


![4_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/4_byte_kind.png)

##### unicode底层存储方式总结

我们现在检验了3种不同的存储方式，到底 cpython 底层会使用多少个字节去存储你的字符串，取决于你字符串里最大的字符的范围，每一个字符在 unicode 对象里占用的空间必须是同样大小的，只有这样才有可能用 O(1) 的时间去获取到unicode对象里任意的第n个字符, 如果 cpython 使用了像 utf8 一样的变长字节数来存储，我们只能做到 O(n) 的时间去获取第 n 个字符

    # 如果按照 utf8 的方式存储 100万个字符，常见字符多的情况下确实会省下一部分空间
	s = "...."
    # 但是下面这个操作无法在 O(1) 时间内完成了
    s[999999]

![kind_overview](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/kind_overview.png)

#### compact

如果 **compact** 设置为 1, 表示所有的字符都存储在了 **PyUnicodeObject** 这一个内存块里，不论 **kind** 为什么值都一样. 上面所有的例子所展示的对象的 **compact** 值都为 1. 如果这个值为 0, data 不再是和 **PyUnicodeObject** 一起的连续的空间，而是单独申请的空间，有点像 redis 里面 **raw** 和 **embstr** 的区别，他们都可以表示 redis 里面的字符串，但是 **raw** 做两次内存分配，**embstr** 做一次

现在我们已经了解了大部分 unicode 的内部实现了，想了解更多细节的同学可以直接参考源码
