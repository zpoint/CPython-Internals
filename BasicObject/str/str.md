# str

# contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [conversion](#conversion)
* [interned](#interned)
* [kind](#kind)
    * [unicode memory usage summary](#unicode-memory-usage-summary)
* [compact](#compact)

# related file
* cpython/Objects/unicodeobject.c
* cpython/Include/unicodeobject.h
* cpython/Include/cpython/unicodeobject.h

# memory layout

You can read [How Python saves memory when storing strings](https://rushter.com/blog/python-strings-and-memory/) first to have an overview of how **unicode** is stored internally

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/layout.png)

for those who are interested in bit-fields in C please refer to [When to use bit-fields in C?](https://stackoverflow.com/questions/24933242/when-to-use-bit-fields-in-c) and [“:” (colon) in C struct - what does it mean?](https://stackoverflow.com/questions/8564532/colon-in-c-struct-what-does-it-mean)

# conversion

Before we look into how unicode objects are created and resized, let's look at the C function **PyUnicode_AsUTF8**

```c
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

```

PyUnicode_UTF8

![pyunicode_utf8](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/pyunicode_utf8.png)

_PyUnicode_UTF8

![_PyUnicode_UTF8.png](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8.png)

_PyUnicode_AsUTF8String

![_PyUnicode_AsUTF8String](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_AsUTF8String.png)

_PyUnicode_UTF8_LENGTH

![_PyUnicode_UTF8_LENGTH](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/_PyUnicode_UTF8_LENGTH.png)


let's initialize an empty string

```python3

# initialize an empty string
s = ""
# notice, because s is compact and ascii, it uses the address of utf8_length as the beginning of the real address

```

![empty_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/empty_s.png)

```c
s = "s"
repr(s) # I've altered the source code as usual
address of char *utf8: 0x1091d6ea0, content of char *utf8: 0x1
hash: 24526920963829810, interned: 1, kind: PyUnicode_1BYTE_KIND,
compact: 1, ready: 1, ascii: 1, ascii length: 1, utf8 length: 115, ready: 0
address of unicode: 0x1091d6e68, PyUnicode_UTF8(unicode): 0x1091d6e98
calling PyUnicode_AsUTF8(unicode): s


```

Have you noticed the field **utf8_length** in **PyCompactUnicodeObject** is 115, and chr(115) is 's'? Yes, this is the beginning address of the C-style null-terminated string

![s_s](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/s_s.png)

```python3

s = "aaa"

```

![aaa](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/aaa.png)

# interned

All unicode objects with "interned" set to 1 will be held in this dictionary.
Any other newly created unicode object with the same value will point to the same object; it acts as a singleton

```c
static PyObject *interned = NULL;

```

If you delete the string and initialize a new string with the same value, the IDs are the same. The first time you created it, it was stored in the **interned** dictionary

```python3
>>> id(s)
4314134768
>>> del s
>>> s = "aaa"
>>> id(s)
4314134768
>>> y = "aaa"
>>> id(y)
4314134768


```

# kind

There are four possible values for the **kind** field in **PyASCIIObject**, indicating how characters are stored in the unicode object internally.

* PyUnicode_WCHAR_KIND

I haven't found a way to define an unicode object represent with **PyUnicode_WCHAR_KIND** in python layer, It may be used in c/c++ level

* PyUnicode_1BYTE_KIND
    * 8 bits/character
    * ascii flag set ?
        * ascii flag set true: U+0000-U+007F
        * ascii flag set false: at least one character in U+0080-U+00FF

The **utf8_length** field still stores the null-terminated C-style string, except the interned field is 0. Only characters in a specific range will cause CPython to store the unicode object in the interned dictionary.

```python3
>>> s1 = "\u007F\u0000"
>>> id(s1)
4472458224
>>> s2 = "\u007F\u0000"
>>> id(s2) # bacause "interned" field is 0, the unicode object will not be shared
4472458608

```

![1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/1_byte_kind.png)

Let's define a unicode object with a character **\u0088** inside

```python3
s = "\u0088\u0011\u00f1"

```

Now, because the first character is **U+0088**, the ascii flag becomes 0, and **PyUnicode_UTF8(unicode)** no longer returns the address of the **utf8_length** field. Instead, it returns the value in the `char *utf8` field, which is 0

If **PyUnicode_UTF8(unicode)** is zero, where are the three bytes located?
We haven't used the data field in **PyUnicodeObject**. Let's print whatever is inside the **data** field.
It took me some time to figure out how to print those bytes in the latin1 field.

```c
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

```

After recompiling the above code, we are able to track latin1 fields in the repr() function

```python3
>>> repr(s)
PyUnicode_1BYTE_KIND,
PyUnicodeObject->latin1: 0x88 0x11 0xf1

```

![no_ascii_1_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/no_ascii_1_byte_kind.png)

* PyUnicode_2BYTE_KIND
    * 16 bits/character
    * all characters are in range U+0000-U+FFFF
    * at least one character is in the range U+0100-U+FFFF

We can use the same code to trace bytes stored in the **data** field. Now the field name is **ucs2** (**ucs2** and **latin1** have different names but the same address; they are inside the same C union structure)

```python3
>>> s = "\u0011\u0111\u1111"
>>> repr(s)
kind: PyUnicode_2BYTE_KIND,  PyUnicodeObject->ucs2: 0x11 0x111 0x1111

```

Now the **kind** field is **PyUnicode_2BYTE_KIND** and it takes 2 bytes to store each character

![2_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/2_byte_kind.png)

* PyUnicode_4BYTE_KIND
    * 32 bits/character
    * all characters are in the range U+0000-U+10FFFF
    * at least one character is in the range U+10000-U+10FFFF

Now the kind field becomes **PyUnicode_4BYTE_KIND**

```python3
>>> s = "\u00ff\U0010FFFF\U00100111\U0010FFF1"
>>> repr(s)
kind: PyUnicode_4BYTE_KIND, PyUnicodeObject->ucs4: 00xff 0x10ffff 0x100111 0x10fff1


```

![4_byte_kind](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/4_byte_kind.png)

## unicode memory usage summary

We now know that there are three kinds of storage mechanisms. How many bytes CPython consumes to store a unicode object depends on the maximum range of your characters. All characters inside the unicode object must be the same size; if CPython used variable-size representation such as UTF-8, it would be impossible to do index operations in O(1) time

```python3
# if represented as utf8 encoding, and stores 1 million variable size characters
s = "...."
# it would be impossible to find s[n] in O(1) time
s[999999]

```

![kind_overview](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/kind_overview.png)

# compact

If the flag in the **compact** field is 1, it means all characters are stored within **PyUnicodeObject**, no matter what the **kind** field is. The examples above all have the **compact** field set to 1. Otherwise, the data block will not be stored inside the **PyUnicodeObject** object directly; the data block will be in a newly malloced location.
The difference between **compact** 0 and **compact** 1 is the same as the difference between Redis string encoding **raw** and **embstr**

Now we understand most of the unicode implementation in CPython. For more detail, you can refer directly to the source code