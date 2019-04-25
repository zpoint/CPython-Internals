# fileio

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
	* [new](#new)
	* [add](#add)
		* [hash collision](#hash-collision)
		* [resize](#resize)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES)
	* [clear](#clear)

#### related file
* cpython/Modules/_io/fileio.c

#### memory layout

![memory layout](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/layout.png)

As [python document](https://docs.python.org/3/library/io.html#raw-file-i-o) said, the **FileIO** object represents an OS-level file containing bytes data


	>>> import io
    >>> f = io.FileIO("./1.txt", "a+")
    >>> f.write(b"hello")
    5

The **fd** field represent the low level file descriptor, the **created** flag is 0, **readable**, **writable**, **appending**, **seekable**, **closefd** are all 1.

**blksize** represent the os-level buffer size in bytes for the current fd

**dict** field stores the user specific **filename**

![1_txt_a+](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/1_txt_a+.png)

	f.close()

