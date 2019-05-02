# fileio

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [overview](#overview)
	* [a+](#a+)
	* [close](#close)
	* [rb](#rb)
	* [fd](#fd)

#### related file
* cpython/Modules/_io/fileio.c

#### memory layout

![memory layout](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/layout.png)

#### overview

##### a+

As [python document](https://docs.python.org/3/library/io.html#raw-file-i-o) said, the **FileIO** object represents an OS-level file containing bytes data

	>>> import io
    >>> f = io.FileIO("./1.txt", "a+")
    >>> f.write(b"hello")
    5

The **fd** field represent the low level file descriptor, the **created** flag is 0, **readable**, **writable**, **appending**, **seekable**, **closefd** are all 1.

**blksize** represent the os-level buffer size in bytes for the current fd

**dict** field stores the user specific **filename**

For those who need the detail of python dict object, please refer to my previous article [dict](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict.md)

![1_txt_a+](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/1_txt_a+.png)

##### close

After call the close method, the **fd** becomes -1, and one more key **__IOBase_closed** inserted to **dict** field

	>>> f.close()

![1_txt_close](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/1_txt_close.png)

##### rb

Now, let's open a file in read only mode

the **fd** and **dict** object are all reused, and **writable**, **appending**, **seekable** field now becomes 0/-1

	>>> f = io.FileIO("../../Desktop/2.txt", "rb")

![2_txt_rb](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/2_txt_rb.png)

##### fd

let's pass a integer to parameter name

	>>> f = open("../../Desktop/2.txt", "rb")
    >>> f.fileno()
    3
    >>> f2 = io.FileIO(3, "r")
	"<_io.FileIO name=3 mode='rb' closefd=True>"

![fd_3](https://github.com/zpoint/CPython-Internals/blob/master/Modules/io/fileio/fd_3.png)

