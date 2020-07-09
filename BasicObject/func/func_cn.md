# func![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/func_cn)

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [字段](#字段)
	* [func_code](#func_code)
	* [func_globals](#func_globals)
	* [func_defaults](#func_defaults)
	* [func_kwdefaults](#func_kwdefaults)
	* [func_closure](#func_closure)
	* [func_doc](#func_doc)
	* [func_name](#func_name)
	* [func_dict](#func_dict)
	* [func_module](#func_module)
	* [func_annotations](#func_annotations)
	* [func_qualname](#func_qualname)

# 相关位置文件
* cpython/Objects/funcobject.c
* cpython/Include/funcobject.h
* cpython/Objects/clinic/funcobject.c.h

# 内存构造

在 python 中, 一切皆对象, 包括函数对象, 一个函数对象在 c 语言中被定义为 **PyFunctionObject**

```python3
def f(a, b=2):
    print(a, b)

>>> type(f)
function

```

类型 **function** 表示用户自定义函数/对象, 对于那种内建函数/对象, 他们是不同的类型, 请参考 [method](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method_cn.md)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/layout.png)

# 字段

我们来看看每个字段在 **PyFunctionObject** 中分别代表什么意思

## func_code

**func_code** 存储了一个 **PyCodeObject** 的实例, 这个实例会保存一段函数代码块中相关的信息

一段代码块相关信息会包括对应 python 虚拟机的指令, 这个函数的参数数量, 参数内容等信息

后续会有文章讲 **PyCodeObject**

```python3
>>> f.__code__
<code object f at 0x1078015d0, file "<stdin>", line 1>

```

## func_globals

这个函数相关联的全局作用域

```python3
>>> type(f.__globals__)
<class 'dict'>
>>> f.__globals__
{'__name__': '__main__', '__doc__': None, '__package__': None, '__loader__': <class '_frozen_importlib.BuiltinImporter'>, '__spec__': None, '__annotations__': {}, '__builtins__': <module 'builtins' (built-in)>, 'f': <function f at 0x10296eac0>}

```

## func_defaults

一个元组对象, 存储了该函数的所有默参数的值

```python3
>>> f.__defaults__
(2,)

```

## func_kwdefaults

**func_kwdefaults** 是一个 python 字典对象, 保存了指定了默认值的 [keyword-only argument](https://www.python.org/dev/peps/pep-3102/)

```python3
def f2(a, b=4, *x, key=111):
    print(a, b, x, key)

>>> f2.__kwdefaults__
{'key': 111}

```

## func_closure

**func_closure** 是一个元组对象, 存储了当前函数所对应的所有闭包层级

```python3
def wrapper(func):
    def func_inside(*args, **kwargs):
        print("calling func", func)
        func(args, kwargs)
        print("call done")
    return func_inside

@wrapper
def my_func():
    print("my func")

>>> my_func.__closure__
(<cell at 0x10911c928: function object at 0x1092b3c40>,)
>>> my_func
<function wrapper.<locals>.func_inside at 0x1092b3b40>
>>> wrapper
<function wrapper at 0x1092b3cc0>

```

我们来看一个有更多 _\_closure_\_ 的示例

```python3

def wrapper2(func1):
    def func_inside1(func2):
        def func_inside2(*args2, **kwargs2):
            print("calling func1", func1)
            r = func1(*args2, **kwargs2)
            print("calling func2", func2)
            r = func2(*args2, **kwargs2)
            print("call done")
            return r
        print("func_inside2.__closure__", func_inside2.__closure__)
        return func_inside2
    print("func_inside1.__closure__", func_inside1.__closure__)
    return func_inside1

@wrapper2
def my_func2():
    print("my func2")

def func3():
    print("func3")

# m = my_func()
inside2 = my_func2(func3)
print("----------------")
inside2()

# output
(<cell at 0x100e69eb8: function object at 0x100e6fea0>,)
func_inside1.__closure__ (<cell at 0x1087e9408: function object at 0x1087f1ae8>,)
func_inside2.__closure__ (<cell at 0x1087e9408: function object at 0x1087f1ae8>, <cell at 0x1087e9498: function object at 0x1087f1bf8>)
----------------
calling func1 <function my_func2 at 0x1087f1ae8>
my func2
calling func2 <function func3 at 0x1087f1bf8>
func3
call done


```

## func_doc

通常, 他是一个用来解释函数作用的 **unicode** 对象

```python3
def f():
    """
    I am the document
    """
    pass

print(f.__doc__)

```

## func_name

这个 **PyFunctionObject** 对象的函数名

```python3
def i_have_a_very_long_long_long_name():
    pass

print(i_have_a_very_long_long_long_name.__name__)

# output
# i_have_a_very_long_long_long_name

```

## func_dict

**func_dict** 是一个字典对象, 存储了这个对象的属性

```python3
>>> f.__dict__
{}
>>> f.a = 3
>>> f.__dict__
{'a': 3}

```

## func_module

**func_module** 表示了这个对象所属的/相关联的模块

```python3
>>> f.__module__
'__main__'
>>> from urllib.parse import urlencode
>>> urlencode.__module__
'urllib.parse'

```

## func_annotations

这是一个比较新的特性, 你可以读一下 [PEP 3107 -- Function Annotations](https://www.python.org/dev/peps/pep-3107/), 上面有更好的例子解释这个字段的作用

```python3
def a(x: "I am a int" = 3, y: "I am a float" = 4) -> "return a list":
	pass

>>> a.__annotations__
{'x': 'I am a int', 'y': 'I am a float', 'return': 'return a list'}

```

## func_qualname

这个字段在内嵌函数表示名称的时候挺管用的, 这个字段还包括了从顶往下的内嵌路径, 你可以通过这个字段了解更多函数定义相关的信息, 可以读下 [PEP 3155 -- Qualified name for classes and functions](https://www.python.org/dev/peps/pep-3155/)

```python3
def f():
	def g():
    	pass
    return g

>>> f.__qualname__
'f'
>>> f().__qualname__
'f.<locals>.g'


```

