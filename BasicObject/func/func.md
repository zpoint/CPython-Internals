# func

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [field](#field)
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

#### related file
* cpython/Objects/funcobject.c
* cpython/Include/funcobject.h
* cpython/Objects/clinic/funcobject.c.h

#### memory layout

every thing is an object in python, including function, a function is defined as **PyFunctionObject** in the c level

the type **function** indicates the user-defined method/classes, for **builtin_function_or_method** please refer to [method](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/method/method.md)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/func/layout.png)

#### field

let's figure out the meaning of each field in the **PyFunctionObject**

##### func_code

**func_code** field stores an instance of **PyCodeObject**, which contains information of a code block

A code block must contains python virtual machine's instruction, argument number, argument
body and etc

I will explain **PyCodeObject** in other article

	def f(a, b=2):
    	print(a, b)

	>>> f.__code__
	<code object f at 0x1078015d0, file "<stdin>", line 1>

##### func_globals

the global namespace attached to the function object

	>>> type(f.__globals__)
	<class 'dict'>
    >>> f.__globals__
    {'__name__': '__main__', '__doc__': None, '__package__': None, '__loader__': <class '_frozen_importlib.BuiltinImporter'>, '__spec__': None, '__annotations__': {}, '__builtins__': <module 'builtins' (built-in)>, 'f': <function f at 0x10296eac0>}

##### func_defaults

a tuple stores all the default argument of the function object

	>>> f.__defaults__
	(2,)

##### func_kwdefaults

field **func_kwdefaults** is a python dictionary, which stores the [keyword-only argument](https://www.python.org/dev/peps/pep-3102/) with default value

    def f2(a, b=4, *x, key=111):
        print(a, b, x, key)

    >>> f2.__kwdefaults__
    {'key': 111}

##### func_closure

the **func_closure** field is a tuple, indicate all the enclosing level of the current function object

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

let's see an example with more _\_closure_\_


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


##### func_doc

usually, it's an **unicode** object for explaintion

    def f():
        """
        I am the document
        """
        pass

    print(f.__doc__)

##### func_name

the name of the **PyFunctionObject** object

    def i_have_a_very_long_long_long_name():
        pass

    print(i_have_a_very_long_long_long_name.__name__)

	# output
    # i_have_a_very_long_long_long_name

##### func_dict

**func_dict** field stores the attribute of the function object

	>>> f.__dict__
	{}
    >>> f.a = 3
    >>> f.__dict__
    {'a': 3}

##### func_module

**func_module** field indicate the module which the **PyFunctionObject** attached to

	>>> f.__module__
	'__main__'
    >>> from urllib.parse import urlencode
    >>> urlencode.__module__
    'urllib.parse'

##### func_annotations

you can read [PEP 3107 -- Function Annotations](https://www.python.org/dev/peps/pep-3107/) for more detail

	def a(x: "I am a int" = 3, y: "I am a float" = 4) -> "return a list":
    	pass

	>>> a.__annotations__
	{'x': 'I am a int', 'y': 'I am a float', 'return': 'return a list'}

##### func_qualname

it's used for nested class/function representation, it contains a dotted path leading to the object from the module top-level, refer [PEP 3155 -- Qualified name for classes and functions](https://www.python.org/dev/peps/pep-3155/) for more detail

	def f():
    	def g():
        	pass
        return g

	>>> f.__qualname__
	'f'
    >>> f().__qualname__
    'f.<locals>.g'


