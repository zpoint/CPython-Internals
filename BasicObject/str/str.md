# str

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [conversion](#conversion)
* [interned](#interned)
* [kind](#kind)
	* [unicode memory usage summary](#unicode-memory-usage-summary)
* [compact](#compact)

#### related file
* cpython/Objects/unicodeobject.c
* cpython/Include/unicodeobject.h
* cpython/Include/cpython/unicodeobject.h

#### memory layout

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/layout.png)

For those who are interested in bit-fields in C please refer to [When to use bit-fields in C?](https://stackoverflow.com/questions/24933242/when-to-use-bit-fields-in-c) and [“:” (colon) in C struct - what does it mean?](https://stackoverflow.com/questions/8564532/colon-in-c-struct-what-does-it-mean)

#### conversion

Before we look into how unicode object create, resize, let's look into the c function **PyUnicode_AsUTF8**

    /* Whenever I try to covert PyUnicodeObject to const char* pointer,
       which can be passed to c printf function
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

![pyunicode_utf8](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/pyunicode_utf8.png)

_PyUnicode_UTF8

![_PyUnicode_UTF8.png](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8.png)

_PyUnicode_AsUTF8String

![_PyUnicode_AsUTF8String](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_AsUTF8String.png)

_PyUnicode_UTF8_LENGTH

![_PyUnicode_UTF8_LENGTH](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8_LENGTH.png)


let's initialize an empty string


	# initialize an empty string
	s = ""
	# notice, because s is compact and ascii, it uses the address of utf8_length as the beginning of real address

![empty_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/empty_s.png)

	s = "s"
    repr(s) # I've altered the source code as usual
    address of char *utf8: 0x1091d6ea0, content of char *utf8: 0x1
    hash: 24526920963829810, interned: 1, kind: PyUnicode_1BYTE_KIND,
    compact: 1, ready: 1, ascii: 1, ascii length: 1, utf8 length: 115, ready: 0
    address of unicode: 0x1091d6e68, PyUnicode_UTF8(unicode): 0x1091d6e98
    calling PyUnicode_AsUTF8(unicode): s


Have you notice the field **utf8_length** in **PyCompactUnicodeObject** is 115, and chr(115) is 's', yes, this is the begin address of the c style null terminate string

![s_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/s_s.png)


	s = "aaa"

![aaa](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/aaa.png)

#### interned

all unicode object with "interned" set to 1 will be hold in this doctionary,
any other new created unicode object with same value will points to same object, it act as singleton

    static PyObject *interned = NULL;

if you delete the string, and initialize a new same string, the id of them are the same, the first time you creted it, it's stored in **interned** dictionary

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

there are totally four values of **kind** field in **PyASCIIObject**, it means how characters are stored in the unicode object internally.

* PyUnicode_WCHAR_KIND

I haven't found a way to define an unicode object represent with **PyUnicode_WCHAR_KIND** in python layer, It may be used in c/c++ level

* PyUnicode_1BYTE_KIND
	* 8 bits/character
	* ascii flag set ?
		* ascii flag set true: U+0000-U+007F
		* ascii flag set false: at least one character in U+0080-U+00FF

the **utf8_length** field still stores the null terminated c style string, except the interned field is 0, only the characters in specific range wiill cpython store the unicode object in the interned dictionary.

	s = "\u007F\u0000"
    id(s)
    4519246896
    del s
    s = "\u007F\u0000" # same id as previous one, even if the interned field is 0
    4519246896
    del s
    gc.collect() # gc.collect also will free oject which field interned is 1
    s = "\u007F\u0000"
    id(s)
    4519245680
    s2 = "\u007F\u0000"
    id(s2)
    4519247152

![1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/1_byte_kind.png)

let's define an unicode object with the a character **\u0088** inside

	s = "\u0088\u0011\u00f1"

Now, because the first character is **U+0088**, the ascii flag becomes 0, and **PyUnicode_UTF8(unicode)** no longer return the address of **utf8_length** field, instead, it returns the value in the **char *utf8** field, and that's 0

if **PyUnicode_UTF8(unicode)** is zero, where is the three bytes located?
we haven't used the data field in **PyUnicodeObject**, let's print whatever inside **data** field.
It took me sometime to figure out how to print those bytes in the latin1 field.

    static PyObject *
    unicode_repr(PyObject *unicode)
    {
    	...
        	/*
            // there exists official marco to get the char in exact index,
            // I use my own to have a better understanding of how things work internally
			Py_ssize_t isize = PyUnicode_GET_LENGTH(unicode);
            Py_ssize_t idata = PyUnicode_DATA(unicode);
            int ikind = PyUnicode_KIND(unicode);
            // no mattner what ikind is, use Py_UCS4(4 bytes) to catch the result
            Py_UCS4 ch = PyUnicode_READ(ikind, idata, i);
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

after recompile with the above code, we are able to track latin1 fields in repr() function

	>>> repr(s)
    PyUnicode_1BYTE_KIND,
	PyUnicodeObject->latin1: 0x88 0x11 0xf1

![no_ascii_1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/no_ascii_1_byte_kind.png)

* PyUnicode_2BYTE_KIND
	* 16 bits/character
	* all characters are in range U+0000-U+FFFF
	* at least one character is in the range U+0100-U+FFFF

we can use same code to trace bytes stored in **data** field, now the field name is **ucs2**(**ucs2** or **latin1** have different name, but same address, they are inside same c union structure)

	>>> s = "\u0011\u0111\u1111"
    >>> repr(s)
    kind: PyUnicode_2BYTE_KIND,  PyUnicodeObject->ucs2: 0x11 0x111 0x1111

now, the **kind** field is **PyUnicode_2BYTE_KIND** and it takes 2 bytes to store each character

![2_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/2_byte_kind.png)

* PyUnicode_4BYTE_KIND
	* 32 bits/character
	* all characters are in the range U+0000-U+10FFFF
	* at least one character is in the range U+10000-U+10FFFF

Now, kind field become **PyUnicode_4BYTE_KIND**

	>>> s = "\u00ff\U0010FFFF\U00100111\U0010FFF1"
    >>> repr(s)
    kind: PyUnicode_4BYTE_KIND, PyUnicodeObject->ucs4: 00xff 0x10ffff 0x100111 0x10fff1


![4_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/4_byte_kind.png)

##### unicode memory usage summary

we now konw that there are three kinds of storage mechanism, how many bytes cpython will consume to store an unicode object depends on the maximum range of your character. All characters inside the unicode object must be in the same size, if cpython use variable size representation such as utf-8, it would be impossible to do index operation in O(1) time

	# if represented as utf8 encoding, and stores 1 million variable size characters
	s = "...."
    # it would be impossible to find s[n] in O(1) time
    s[999999]

![kind_overview](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/kind_overview.png)

#### compact

if flag in **compact** field in 1, it means all characters are stored within **PyUnicodeObject**, no matter what **kind** field is. The example above all have **compact** field set to 1. Otherwise, the data block will not stored inside the **PyUnicodeObject** object directly, the data block will be a newly malloced location.
The difference bwtween **compact** 0 and **compact** 1 is the same as the difference bwtween redis string encoding **raw** and **embstr**

now, we understand most of the unicode implementations in cpython, for more detail, you can directly refer to source code