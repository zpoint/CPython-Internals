# frame

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
	* [f_valuestack/f_stacktop/f_localsplus](#f_valuestack/f_stacktop/f_localsplus)

#### related file
* cpython/Objects/frameobject.c
* cpython/Include/frameobject.h

#### memory layout

the **PyFrameObject** is the stack frame in python virtual machine, it contains space for the current executing code object, parameters, variables in different scope, try block info and etc

for more information please refer to [stack frame strategy](http://en.citizendium.org/wiki/Memory_management)

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/frame/layout.png)

#### example

every time you make a function call, a new **PyFrameObject** will be created and attached to the current function call

it's not intuitive to trace a frame object in the middle of a function, I will use a [generator object](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/gen/gen.md) to do the explanation

you can always get the frame of the current environment by executing `sys._current_frames()`


##### f_valuestack/f_stacktop/f_localsplus

**PyFrameObject** object is variable-sized object, 

