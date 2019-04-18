# set

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
	* [new](#new)
	* [add](#add)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES?)
	* [clear](#clear)

#### related file
* cpython/Objects/unicodeobject.c
* cpython/Include/unicodeobject.h
* cpython/Include/cpython/unicodeobject.h

#### memory layout

![layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/layout.png)

For those who are interested in bit-fields in C please refer to [When to use bit-fields in C?](https://stackoverflow.com/questions/24933242/when-to-use-bit-fields-in-c) and [“:” (colon) in C struct - what does it mean?](https://stackoverflow.com/questions/8564532/colon-in-c-struct-what-does-it-mean)

#### conversion

Before we look into how unicode object create, resize, let's look into the c function **PyUnicode_AsUTF8**

    /* Whenever I try to covert some python object to const char* pointer,
       which can pass to printf function
       I call
       const char *s = PyUnicode_AsUTF8(py_object_to_be_converted)
       let's look at the defination of the function
    */
    const char *
	PyUnicode_AsUTF8(PyObject *unicode)
	{
    	return PyUnicode_AsUTF8AndSize(unicode, NULL);
	}

    /* let's find PyUnicode_AsUTF8AndSize */
    const char *
    PyUnicode_AsUTF8AndSize(PyObject *unicode, Py_ssize_t *psize)
    {
        PyObject *bytes;

		/* do some checking here */

		/* PyUnicode_UTF8 checks whether the unicode object is Compact unicode
           PyUnicode_UTF8(unicode) will return a pointer to char
        */
        if (PyUnicode_UTF8(unicode) == NULL) {
            assert(!PyUnicode_IS_COMPACT_ASCII(unicode));
            /* bytes should be a PyBytesObject, there are 4 situations totally */
            bytes = _PyUnicode_AsUTF8String(unicode, NULL);
            if (bytes == NULL)
                return NULL;
            /* set ((PyCompactUnicodeObject*)(unicode))->utf8 to new malloced buffer size */
            _PyUnicode_UTF8(unicode) = PyObject_MALLOC(PyBytes_GET_SIZE(bytes) + 1);
            if (_PyUnicode_UTF8(unicode) == NULL) {
            	/* check for fail malloc */
                PyErr_NoMemory();
                Py_DECREF(bytes);
                return NULL;
            }
            /* set ((PyCompactUnicodeObject*)(unicode))->utf8_length to length of bytes */
            _PyUnicode_UTF8_LENGTH(unicode) = PyBytes_GET_SIZE(bytes);
            /* copy the real bytes from bytes to unicode */
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

![pyunicode_utf8](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/pyunicode_utf8.png)

_PyUnicode_UTF8

![_PyUnicode_UTF8.png](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8.png)

_PyUnicode_AsUTF8String

![_PyUnicode_AsUTF8String](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/_PyUnicode_AsUTF8String.png)

_PyUnicode_UTF8_LENGTH

![_PyUnicode_UTF8_LENGTH](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8_LENGTH.png)

#### method

* ##### **new**
    * call stack
	    static PyUnicodeObject *_PyUnicode_New(Py_ssize_t length)

now, let's see the method of unicode object

	# initialize an empty string
	s = ""

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