# func

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [field](#field)
	* [func_code](#func_code)


#### related file
* cpython/Objects/funcobject.c
* cpython/Include/funcobject.h
* cpython/Objects/clinic/funcobject.c.h

#### memory layout

Every thing is an object in python, including function, a function is defined as **PyFunctionObject** in the c level

![layout](https://github.com/zpoint/Cpython-Internals/blob/master/BasicObject/func/layout.png)

#### field

Let's figure out the meaning of each field in the **PyFunctionObject**

##### func_code

**func_code** field stores an instance of **PyCodeObject**, which contains information of a code block

A code block must contains python virtual machine's instruction, argument number, argument
body and etc

I will explain **PyCodeObject** in other article

	def f(a, b=2):
    	print(a, b)

	>>> f.__code__
	<code object f at 0x1078015d0, file "<stdin>", line 1>