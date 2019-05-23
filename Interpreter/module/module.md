# module

### contents

* [related file](#related-file)
* [memory layout](#memory-layout)
* [example](#example)
* [how does import work](#how-does-import-work)

#### related file

* cpython/Objects/moduleobject.c
* cpython/Include/moduleobject.h
* cpython/Objects/clinic/moduleobject.c.h

#### memory layout

there's a struct named **PyModuleDef** defined in `Include/moduleobject.h`

![layout_PyModuleDef](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleDef.png)

the **PyModuleObject** is defined in `Objects/moduleobject.c`, which contains a field with type **PyModuleDef**

![layout_PyModuleObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleObject.png)

#### example

