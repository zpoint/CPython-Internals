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

let's compile a script with only one line `from urllib.parse import urlencode`

    ./python.exe -m dis test.py
      1           0 LOAD_CONST               0 (0)
                  2 LOAD_CONST               1 (('urlencode',))
                  4 IMPORT_NAME              0 (urllib.parse)
                  6 IMPORT_FROM              1 (urlencode)
                  8 STORE_NAME               1 (urlencode)
                 10 POP_TOP
                 12 LOAD_CONST               2 (None)
                 14 RETURN_VALUE

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

	/* cpython/Python/import.c */
    PyObject *
    PyImport_ImportModuleLevelObject(PyObject *name, PyObject *globals,
                                     PyObject *locals, PyObject *fromlist,
                                     int level)
    {
        /* do some check here */
        /* omit */
        /* check if it's already imported, if so, mod will be the imported module */
        mod = PyImport_GetModule(abs_name);
        if (mod != NULL && mod != Py_None) {
        	/* if it's already imported, check if the module needs to be initialized */
            _Py_IDENTIFIER(__spec__);
            _Py_IDENTIFIER(_lock_unlock_module);
            PyObject *spec;

            /* Optimization: only call _bootstrap._lock_unlock_module() if
               __spec__._initializing is true.
               NOTE: because of this, initializing must be set *before*
               stuffing the new module in sys.modules.
             */
            spec = _PyObject_GetAttrId(mod, &PyId___spec__);
            if (_PyModuleSpec_IsInitializing(spec)) {
                PyObject *value = _PyObject_CallMethodIdObjArgs(interp->importlib,
                                                &PyId__lock_unlock_module, abs_name,
                                                NULL);
                if (value == NULL) {
                    Py_DECREF(spec);
                    goto error;
                }
                Py_DECREF(value);
            }
            Py_XDECREF(spec);
        }
        else {
            Py_XDECREF(mod);
            /* the real import procedure */
            /* after various call stack, it will call builtin___import__ defined in cpython/Python/bltinmodule.c */
            mod = import_find_and_load(abs_name);
            if (mod == NULL) {
                goto error;
            }
        }

        has_from = 0;
        if (fromlist != NULL && fromlist != Py_None) {
            has_from = PyObject_IsTrue(fromlist);
            if (has_from < 0)
                goto error;
        }
        if (!has_from) {
            Py_ssize_t len = PyUnicode_GET_LENGTH(name);
            if (level == 0 || len > 0) {
                Py_ssize_t dot;

                dot = PyUnicode_FindChar(name, '.', 0, len, 1);
                if (dot == -2) {
                    goto error;
                }

                if (dot == -1) {
                    /* No dot in module name, simple exit */
                    final_mod = mod;
                    Py_INCREF(mod);
                    goto error;
                }

                if (level == 0) {
                    PyObject *front = PyUnicode_Substring(name, 0, dot);
                    if (front == NULL) {
                        goto error;
                    }

                    final_mod = PyImport_ImportModuleLevelObject(front, NULL, NULL, NULL, 0);
                    Py_DECREF(front);
                }
                else {
                    Py_ssize_t cut_off = len - dot;
                    Py_ssize_t abs_name_len = PyUnicode_GET_LENGTH(abs_name);
                    PyObject *to_return = PyUnicode_Substring(abs_name, 0,
                                                            abs_name_len - cut_off);
                    if (to_return == NULL) {
                        goto error;
                    }

                    final_mod = PyImport_GetModule(to_return);
                    Py_DECREF(to_return);
                    if (final_mod == NULL) {
                        PyErr_Format(PyExc_KeyError,
                                     "%R not in sys.modules as expected",
                                     to_return);
                        goto error;
                    }
                }
            }
            else {
                final_mod = mod;
                Py_INCREF(mod);
            }

      error:
        Py_XDECREF(abs_name);
        Py_XDECREF(mod);
        Py_XDECREF(package);
        if (final_mod == NULL)
            remove_importlib_frames();
        return final_mod;
    }