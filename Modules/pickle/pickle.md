# pickle

# contents

* [related file](#related-file)
* [introduction](#introduction)
* [implementation](#implementation)
  * [None](#None)
  * [bool](#bool)
  * [int](#int)
  * [float](#float)
  * [bytes](#bytes)
  * [str](#str)
  * [tuple](#tuple)
  * [list](#list)
  * [type](#type)
  * [object](#object)

# related file

* Lib/pickle.py
* Modules/_pickle.c
* Modules/clinic/_pickle.c.h

# introduction

We use the `pickle` module to serialize and deserialize our Python objects. There are several protocol versions for `pickle`. The current version is `4`

 The `pickle` module will use the faster  `_pickle` implemented in `C`(`Modules/_pickle.c`)  if possible, if the `_pickle` module not found, the `python` implemented `pickle`(`Lib/pickle.py`) will be used

|     Type     | Implementation |
| :----------: | :------------: |
|     None     |   save_none    |
|     bool     |   save_bool    |
|     int      |   save_long    |
|    float     |   save_float   |
|    bytes     |   save_bytes   |
|     str      |    save_str    |
|    tuple     |   save_tuple   |
|     list     |   save_list    |
|     dict     |   save_dict    |
|     set      |    save_set    |
|  frozenset   | save_frozenset |
| FunctionType |  save_global   |
|              |  save_reduce   |

# implementation

Whenever you call dump, some extra information will be added to the result

The first byte is an identifier indicate that the following binary content is encoded in "pickle protocol"

The second byte is the protocol version

The final byte is a stop symbol indicate that it's the end of the binary content

![pickle_head](pickle_head.png)



## None

```python3
NONE = b'N'   # push None

def save_none(self, obj):
	self.write(NONE)
```

The `data` is `N` here, with the aforementioned information added to it

```python3
>>> import pickle
>>> pickle.dumps(None)
b'\x80\x04N.'
```

## bool

`bool` is similar to `None`

```python3
NEWTRUE        = b'\x88'  # push True
NEWFALSE       = b'\x89'  # push False

def save_bool(self, obj):
	if self.proto >= 2:
		self.write(NEWTRUE if obj else NEWFALSE)
```

The `data` here is `b'\x88'(True)` and `b'\x89'(False)`

```python3
>>> import pickle
>>> pickle.dumps(True)
b'\x80\x04\x88.'
>>> pickle.dumps(False)
b'\x80\x04\x89.'
```

## int

The integer will be saved in various formats according to its value

![int](int.png)

![int2](int2.png)



## float

The float is saved in [IEEE_754](https://en.wikipedia.org/wiki/IEEE_754-1985) standard

![float](float.png)

## bytes

`bytes` object is saved directly as the `data` part below

The `head` part varies according to the data size

![bytes](bytes.png)

## str

`str` is similar to [bytes](#bytes),  except that `str` is encoded in `utf-8` format before dump

![str](str.png)

## tuple

`tuple` is more complicated than other basic type

If the `tuple` is empty

![tuple0](tuple0.png)

Let's see an example

```python3
dumps(("a", "b", (2, )))
b'\x80\x04\x95\x0f\x00\x00\x00\x00\x00\x00\x00\x8c\x01a\x94\x8c\x01b\x94K\x02\x85\x94\x87\x94.'
```

`\x80\x04` is pickle protocol and pickle version

`\x95\x0f\x00\x00\x00\x00\x00\x00\x00` is frame symbol(`\x95`) and frame size(8 bytes) in little endian

`.` in last byte is the `STOP` symbol

Besides are the data

 ![tuple1](tuple1.png)

I found that dumps does not support self-referencing tuples(how to [Build Self-Referencing Tuples](https://stackoverflow.com/questions/11873448/building-self-referencing-tuples))

## list

Let's see an example again

```python3
dumps(["a", "b", (2, )])
b'\x80\x04\x95\x11\x00\x00\x00\x00\x00\x00\x00]\x94(\x8c\x01a\x94\x8c\x01b\x94K\x02\x85\x94e.'
```

The first several bytes are pickle protocol, pickle version and frame size

The last byte is `STOP` symbol

The data can be described as (`]\x94(\x8c\x01a\x94\x8c\x01b\x94K\x02\x85\x94e`)

`list` will be dumped batch by batch(default batch size `1000`)

![list1](./list1.png)

`dict` and `set` are similar to `list` and `tuple`, beginning and ending with a `type` symbol to indicate the type, and iterating through each object and recursively calling `dump` for each object

## type

If what's to be saved is a type

```python3
class A(object):
    a = "a"
    b = "b"

    def run(self):
        print(self.a, self.b)

pickle.dumps(A)
b'\x80\x04\x95\x12\x00\x00\x00\x00\x00\x00\x00\x8c\x08__main__\x94\x8c\x01A\x94\x93\x94.'
```

  The data part is `\x8c\x08__main__\x94\x8c\x01A\x94\x93\x94`

![type1](./type1.png)

`dumps(A)` saves the `module_name` (`__main__`) and the `object` name (`A`) in [str](#str) format

## object

If what's to be saved is an instance

```python3
a = A()
pickle.dumps(a)
b'\x80\x04\x95\x15\x00\x00\x00\x00\x00\x00\x00\x8c\x08__main__\x94\x8c\x01A\x94\x93\x94)\x81\x94.'
```

The data part is `\x8c\x08__main__\x94\x8c\x01A\x94\x93\x94)\x81\x94`

The only difference is that there is some extra information appended after the previous dumped result

A `TUPLE` indicate the `args` needed for instance call, in the current case `a = A()` the args is empty, so it's an `EMPTY_TUPLE`

A `NEWOBJ` symbol indicate that it needs to call `cls.__new__(cls, *args)` after load the dumped result

![object1](./object1.png)

