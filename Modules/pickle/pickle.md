# Pickle

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
* [read more](#read-more)

# related file

* Lib/pickle.py
* Modules/_pickle.c
* Modules/clinic/_pickle.c.h

# introduction

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

`bool` is simiiar to `None`

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

The integer will be saved in various format according to it's value

![int](int.png)

![int2](int2.png)



## float

The float is saved in [IEEE_754](https://en.wikipedia.org/wiki/IEEE_754-1985) standard

![float](float.png)

## bytes

``bytes` object is save directly as the `data` part below

The `head` part various according to the data size

![bytes](bytes.png)

## str

`str` is similar to [bytes](#bytes),  except that `str` is encoded in `utf-8` format before dump

![str](str.png)

## tuple

`tuple` is more complicated than other basic type

If the `tuple` is empty

![tuple0](tuple0.png)

If 

# read more

