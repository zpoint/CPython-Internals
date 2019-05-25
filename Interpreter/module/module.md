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
* cpython/Python/import.c
* cpython/Python/clinic/import.c.h

#### memory layout

there's a struct named **PyModuleDef** defined in `Include/moduleobject.h`

![layout_PyModuleDef](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleDef.png)

the **PyModuleObject** is defined in `Objects/moduleobject.c`, which contains a field with type **PyModuleDef**

![layout_PyModuleObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleObject.png)

#### example

the field **md_dict** is the `__dict__` attribute of the module object

**PyModuleDef** is optional, the index located in the m_base field is used for find the module by index in sys.modules, not by name

**m_size** stores the size of per-module data

**m_clear** and **m_free** are used for deallolcation

for more detail please refer to [PEP 3121 -- Extension Module Initialization and Finalization](https://www.python.org/dev/peps/pep-3121/)

    import _locale

![locale](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/locale.png)

    import re

![re](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/re.png)

#### how does import work

let's compile a script with one line `import re`

    ./python.exe -m dis test.py
      1           0 LOAD_CONST               0 (0)
                  2 LOAD_CONST               1 (None)
                  4 IMPORT_NAME              0 (re)
                  6 STORE_NAME               0 (re)
                  8 LOAD_CONST               1 (None)
                 10 RETURN_VALUE

the core opcde here is `IMPORT_NAME`

    case TARGET(IMPORT_NAME): {
        PyObject *name = GETITEM(names, oparg);
        PyObject *fromlist = POP();
        PyObject *level = TOP();
        PyObject *res;
        res = import_name(f, name, fromlist, level);
        Py_DECREF(level);
        Py_DECREF(fromlist);
        SET_TOP(res);
        if (res == NULL)
            goto error;
        DISPATCH();
    }