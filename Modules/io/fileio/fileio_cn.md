# fileio

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [总览](#总览)
	* [a+](#a+)
	* [close](#close)
	* [rb](#rb)
	* [fd](#fd)

#### 相关位置文件
* cpython/Modules/_io/fileio.c

#### 内存构造

![memory layout](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/layout.png)

#### 总览

##### a+

如 [python官方文档](https://docs.python.org/3/library/io.html#raw-file-i-o) 所说 **FileIO** 对象表示了一个在操作系统级别上对字节对象进行操作的文件对象

	>>> import io
    >>> f = io.FileIO("./1.txt", "a+")
    >>> f.write(b"hello")
    5

**fd** 这个字段表示文件描述符号, **created** 这个字段是 0, **readable**, **writable**, **appending**, **seekable**, **closefd** 都是 1

**blksize** 表示当前文件描述符对应的操作系统的缓冲区大小, 单位是字节

**dict** 对象存储了一些相关信息, 这里保存了用户传入的 **filename** 的内容

对 python **dict** 对象有兴趣的读者, 请参考我以前的文章 [dict](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/dict/dict_cn.md)

![1_txt_a+](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/1_txt_a+.png)

##### close

调用 **close** 方法后, **fd** 字段变成了 -1, 并且 键 **__IOBase_closed** 被插入了当前的 **dict** 里面

	>>> f.close()

![1_txt_close](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/1_txt_close.png)

##### rb

现在我们来试试以只读方式打开一个文件

**fd** 和 **dict** 都没有发生改变, 说明他们被循环利用了, 并且 **writable**, **appending**, **seekable** 的值现在变成了 0/-1

	>>> f = io.FileIO("../../Desktop/2.txt", "rb")

![2_txt_rb](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/2_txt_rb.png)

##### fd

我们传入一个文件描述符号试一试

	>>> f = open("../../Desktop/2.txt", "rb")
    >>> f.fileno()
    3
    >>> f2 = io.FileIO(3, "r")
	"<_io.FileIO name=3 mode='rb' closefd=True>"

![fd_3](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/fd_3.png)

