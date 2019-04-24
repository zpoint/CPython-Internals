# bytes

### 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
	* [empty bytes](#empty-bytes)
	* [ascii characters](#ascii-characters)
	* [nonascii characters](#nonascii-characters)
* [总结](#总结)
	* [ob_shash](#ob_shash)
	* [ob_size](#ob_size)
	* [总结](#总结)

#### 相关位置文件
* cpython/Objects/bytesobject.c
* cpython/Include/bytesobject.h
* cpython/Objects/clinic/bytesobject.c.h

#### 内存构造

![memory layout](https://img-blog.csdnimg.cn/20190318160629447.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMxNzIwMzI5,size_16,color_FFFFFF,t_70)

**PyBytesObject** 的内存构造和 [tuple的内存构造](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/tuple/tuple_cn.md#%E5%86%85%E5%AD%98%E6%9E%84%E9%80%A0)/[int的内存构造](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/long/long_cn.md#%E5%86%85%E5%AD%98%E6%9E%84%E9%80%A0) 非常相似, 但是实现起来比上面的其他元素要简单一些

#### 示例

##### empty bytes

**bytes** 对象在 python 中是不可变的对象, 每当你需要更改 **bytes** 对象里的字符的时候, 你需要创建一个新的 **bytes** 对象, 无法在原来的基础上修改, 这样简化了 **bytes** 对象的实现

	s = b""

![empty](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytes/empty.png)

##### ascii characters

我们来初始化一个只包含 ascii 字符的 bytes 对象

	s = b"abcdefg123"

![ascii](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytes/ascii.png)

##### nonascii characters

	s = "我是帅哥".encode("utf8")

![nonascii](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/bytes/nonascii.png)

#### 总结


##### ob_shash

**ob_shash** 存储的是这一串 bytes 字符的哈希值(这个bytes对象的哈希值), 值 **-1** 表示这个哈希值还未计算过

第一次你需要用到这个值的时候, 哈希值会被计算出来并存储到 **ob_shash** 这个坑里

缓存哈希值可以避免重复的计算, 并提高字典查询的速度

##### ob_size

**ob_size** 这个字段是跟在 **PyVarObject** 结构里的, **PyBytesObject** 使用这个字段来存储当前保存的 bytes array 的大小(不包括最后的\0结束字符), 这样可以在 O(1) 的时间复杂度下完成 **len()** 这类查询操作, 并且当c数组里存储的是非ascii字符时, 无法确定遇到的\0是表示数组结束, 还是只是其中的一个字符, 只能通过 **ob_size** 来记录

##### 总结

**PyBytesObject** 其实就是 python 把 c 的字符数组做了一层包装, 带上了一个 **ob_shash** 用来存储哈希值, **ob_size** 用来存储 **PyBytesObject** 的 c 字符数组的大小

他的实现方式和 redis 里面的 **embstr** 实现方式类似

	redis-cli
    127.0.0.1:6379> set a "hello"
    OK
    127.0.0.1:6379> object encoding a
    "embstr"
