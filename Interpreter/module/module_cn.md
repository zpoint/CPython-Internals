# module![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/module_cn)

# 目录

* [相关位置文件](#相关位置文件)
* [内存构造](#内存构造)
* [示例](#示例)
* [如何调试 import](#如何调试-import)
* [import 是如何实现的](#import-是如何实现的)
	* [finder](#finder)
        * [BuiltinImporter](#BuiltinImporter)
        * [FrozenImporter](#FrozenImporter)
        * [PathFinder](#PathFinder)

# 相关位置文件

* cpython/Objects/moduleobject.c
* cpython/Include/moduleobject.h
* cpython/Objects/clinic/moduleobject.c.h
* cpython/Python/import.c
* cpython/Python/clinic/import.c.h
* cpython/Lib/importlib/_bootstrap.py
* cpython/Lib/importlib/_bootstrap_external.py

# 内存构造

`Include/moduleobject.h` 中定义了一个 c struct, 名为 **PyModuleDef**

![layout_PyModuleDef](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleDef.png)

`Objects/moduleobject.c` 中又定义了 **PyModuleObject** 这个结构, 它里面有个类型为 **PyModuleDef** 的字段

![layout_PyModuleObject](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/layout_PyModuleObject.png)

# 示例

字段 **md_dict** 是该 module 对象的 `__dict__` 属性

**PyModuleDef** 是可选的, m_base 字段中的 index 值是用来在 sys.modules 中以下标的方式搜索该模块用的, 而不需要遍历整个数组匹配名称来搜索

**m_size** 存储了每个 module 需要使用到的额外数据的大小

**m_clear** 和 **m_free** 是在 module 对象 删除/释放时和 c++ 中析构函数的作用类似

更多详情请参考 [PEP 3121 -- Extension Module Initialization and Finalization](https://www.python.org/dev/peps/pep-3121/)

```python3
import _locale

```

![locale](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/locale.png)

```python3
import re

```

![re](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/re.png)

# 如何调试 import

当你尝试到源码里面看看 `import` 是如何工作时

跟着函数的调用栈

![import_call_stack](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/import_call_stack.png)

你可以发现, 核心的函数是 `interp->importlib`, 这个函数是在下面这个位置初始化的

```c
/* cpython/Python/pylifecycle.c */
static _PyInitError
initimport(PyInterpreterState *interp, PyObject *sysmod)
{
    /* 忽略 */
    importlib = PyImport_AddModule("_frozen_importlib");
    if (importlib == NULL) {
        return _Py_INIT_ERR("couldn't get _frozen_importlib from sys.modules");
    }
    interp->importlib = importlib;
    Py_INCREF(interp->importlib);
    /* 忽略 */
}

```

搜索 `_frozen_importlib`, 你可以找到一个几乎是二进制的文件 `Python/importlib.h`, 里面的内容如下

```c
/* 被 Programs/_freeze_importlib.c 这个程序自动生成的 */
const unsigned char _Py_M__importlib_bootstrap[] = {
    99,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,
    ...
}
*/

```

我们发现 `_freeze_importlib.c` 编译后的程序会把 `Lib/importlib/_bootstrap.py` 中的 python 代码转换为 二进制格式并存储在 `Python/importlib.h` 中

![importlib](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/importlib.png)

到这里我们发现了一个比较有意思的地方, 整个 `import` 的流程其实大致上是用 python 代码完成的, `interp->importlib` 中的 `PyId__find_and_load` 属性能映射到 `Lib/importlib/_bootstrap.py` 文件中的 `_find_and_load` python 函数

当你更改了 `Lib/importlib/_bootstrap.py` 中的代码时, 你需要重新生成 `Python/importlib.h` 这个文件, 并重新编译生成一个新的可执行程序 `python.exe`

```python3
# 编译 ./Programs/_freeze_importlib.c, 用编译后的程序去做下面的转换
# _bootstrap_external.py -> importlib_external.h
# _bootstrap.py -> importlib.h
# zipimport.py -> importlib_zipimport.h
make regen-importlib
# 重新编译生成新的可执行 `python.exe`
make

```

# import 是如何实现的

我们用 dis 模块处理一个只有一行代码 `import _locale` 的脚本看看

```python3
./python.exe -m dis test.py
1           0 LOAD_CONST               0 (0)
          2 LOAD_CONST               1 (None)
          4 IMPORT_NAME              0 (_locale)
          6 STORE_NAME               0 (_locale)
          8 LOAD_CONST               1 (None)
         10 RETURN_VALUE

```

这里核心的 opcde 是 `IMPORT_NAME`

把所有的 `IMPORT_NAME` 调用到的并且相关的代码拷贝过来并写上注释看起来也很让人头大, 这里不这么做

想象以下当前有两个以上的线程在导入同一个 `_locale` 对象, CPython 如何处理这种情况 ?

如果你读了源代码和下面的图片, 你会看到这里使用了锁机制来防止竞争的情况发生

整个过程大概如下

1. opcode `IMPORT_NAME` 会检查 sys.module 中是否包含有要导入的模块的名称, 如果有直接返回这个对象
2. 尝试获得 `_imp` 这把锁
3. 获取到 `_module_locks` 里面对应模块名称的值, 如果名称不在 `_module_locks` 里, 则创建一个新的对象 (`position 1` 的位置)
4. 对步骤 3 获得的对象上锁 (在 `position 2` 的位置)
5. 释放 `_imp` 上的锁 (在 `position 3` 的位置)
6. 再次检查是否即将导入的名称在 sys.module 中, 如果在释放 `_module_locks` 获得的锁并结束 (在 `position 4` 的位置)
7. 对于 `sys.meta_path` 中的每个 `finder`, 如果 `finder` 能成功导入, 则释放 `_module_locks` 获得的锁并结束
8. 抛出无法找到的异常

在 `position 1`, 只有获得了 `_imp` 上面的锁的线程可以更改 `_module_locks` 里的值, 当前的线程会检查当前要导入的名称在不在 `_module_locks` 这个字典中, 如果不在就往这个字典插入一个名称为当前模块, 值为锁对象

在 `position 3`, `_imp` 已经被释放了, 如果其他的线程在尝试导入其他的模块, 这里是不会相互影响的, 其他的线程可以获得 `_imp` 这把锁并继续同样的操作, 如果其他的线程尝试导入相同的模块, 即使获得了 `_imp` 锁, 获取 `_module_locks` 里面的锁的时候也会失败, 因为当前的线程已经拥有了这个锁了, 某种意义上, `_module_locks` 中存储的是比 `_imp` 更细粒度的锁

![import procedure1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/import%20procedure1.png)

在 `position 4` 的位置, 当前的线程会再检查一遍 `sys.modules`

在 `position 5` 的位置, 每一个 `finder.find` 之前都会获得 `_imp` 锁, 并在 `finder.find` 之后释放掉

![import procedure2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/module/import%20procedure2.png)

## finder

默认情况下 `sys.meta_path` 只有 3 个不同的 finder

```python3
>>> sys.meta_path
[<class '_frozen_importlib.BuiltinImporter'>, <class '_frozen_importlib.FrozenImporter'>, <class '_frozen_importlib_external.PathFinder'>]

```

### BuiltinImporter

`BuiltinImporter` 会处理所有的 built-in 模块, 比如当我们运行

```python3
import _locale

```

`BuiltinImporter` 定义的位置是在 `Lib/importlib/_bootstrap.py`, `BuiltinImporter.find_spec` 会返回一个 `ModuleSpec` 对象, `ModuleSpec` 有一个 `loader` 属性, 在返回的时候这个 `loader` 已经和 `BuiltinImporter` 绑定在一起了

之后 `ModuleSpec.loader.create_module` 就会被调用, 最终调用到了 `_imp_create_builtin` 这个 c 函数上

```c
/* 在 cpython/Python/import.c 这个位置 */
static PyObject *
_imp_create_builtin(PyObject *module, PyObject *spec)
{
    /* 一些类型检查 */
    PyObject *modules = NULL;
    for (p = PyImport_Inittab; p->name != NULL; p++) {
    	/* PyImport_Inittab 是一个 c 数组, 每一个元素都是一个 built-in module 名称 和 初始化的 c 函数
        这里的 for 循环遍历 PyImport_Inittab 这个数组, 检查他们的名称和你提供的名称是否匹配,
        如果匹配直接调用对应的初始化的 c 函数并返回m*/
        PyModuleDef *def;
        if (_PyUnicode_EqualToASCIIString(name, p->name)) {
            if (p->initfunc == NULL) {
                /* Cannot re-init internal module ("sys" or "builtins") */
                mod = PyImport_AddModule(namestr);
                Py_DECREF(name);
                return mod;
            }
            mod = (*p->initfunc)();
            /* 一些类型检查 */
        }
    }
    Py_DECREF(name);
    Py_RETURN_NONE;
}

/* in cpython/PC/config.c
struct _inittab _PyImport_Inittab[] = {

    {"_abc", PyInit__abc},
    {"array", PyInit_array},
    {"_ast", PyInit__ast},
    {"audioop", PyInit_audioop},
    {"binascii", PyInit_binascii},
    {"cmath", PyInit_cmath},
    {"errno", PyInit_errno},
    {"faulthandler", PyInit_faulthandler},
    {"gc", PyInit_gc},
    {"math", PyInit_math},
    {"nt", PyInit_nt}, /* Use the NT os functions, not posix */
    {"_operator", PyInit__operator},
    {"_signal", PyInit__signal},
    {"_md5", PyInit__md5},
    ...
}
*/

```

### FrozenImporter

`FrozenImporter.loader.create_module` 的定义, 调用方式等和 `BuiltinImporter` 类似

```c
int
PyImport_ImportFrozenModuleObject(PyObject *name)
{
    p = find_frozen(name);
    /* 类型检查 */
    co = PyMarshal_ReadObjectFromString((const char *)p->code, size);
    /* 类型检查 */
    d = module_dict_for_exec(name);
    /* 类型检查 */
    m = exec_code_in_module(name, d, co);
    if (m == NULL)
        goto err_return;
    Py_DECREF(co);
    Py_DECREF(m);
    return 1;
err_return:
    Py_DECREF(co);
    return -1;
}


static const struct _frozen *find_frozen(PyObject *name)
{
	/* 类型检查 */
    for (p = PyImport_FrozenModules; ; p++) {
    	/* 遍历 PyImport_FrozenModules 这个 c 数组检查是否有名称匹配的对象 */
        if (p->name == NULL)
            return NULL;
        if (_PyUnicode_EqualToASCIIString(name, p->name))
            break;
    }
    return p;
}

/* 在 cpython/Python/frozen.c 这个位置
static const struct _frozen _PyImport_FrozenModules[] = {
    /* importlib */
    {"_frozen_importlib", _Py_M__importlib_bootstrap,
        (int)sizeof(_Py_M__importlib_bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib_bootstrap_external,
        (int)sizeof(_Py_M__importlib_bootstrap_external)},
    {"zipimport", _Py_M__zipimport,
        (int)sizeof(_Py_M__zipimport)},
    /* Test module */
    {"__hello__", M___hello__, SIZE},
    /* Test package (negative size indicates package-ness) */
    {"__phello__", M___hello__, -SIZE},
    {"__phello__.spam", M___hello__, SIZE},
    {0, 0, 0} /* sentinel */
};

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
*/

```

### PathFinder

`PathFinder.loader.create_module` 会最终调用 `_new_module` 返回, 而不是像上面的调用 `create_module` 返回, 外层函数定义的位置在 `cpython/Lib/importlib/_bootstrap.py`, `_new_module` 不会调用 c 函数, `create_module` 会调用对应的 c 函数

```python3
def module_from_spec(spec):
    """通过提供的 spec 生成一个  module 对象"""
    # 典型的 loader 是不会提供 create_module() 函数的
    module = None
    if hasattr(spec.loader, 'create_module'):
        # BuiltinImporter and FrozenImporter 会在这里调用 create_module
        # 最后跑到他们的 c function 中去
        module = spec.loader.create_module(spec)
    elif hasattr(spec.loader, 'exec_module'):
        raise ImportError('loaders that define exec_module() '
                          'must also define create_module()')
    if module is None:
    	# PathFinder 会到达这里
        # _new_module 简单的返回了一个基本的 module 类型对象
        # 加载的过程都是通过参数 spec 完成的
        module = _new_module(spec.name)
    _init_module_attrs(spec, module)
    return module

def _new_module(name):
    return type(sys)(name)

```

`PathFinder.find_spec` 会把 `sys.path_hooks` 中的对象取出来, 并按 `sys.path_hooks` 中的对象的顺序, 把每个对象作为 finder 去尝试进行模块导入

`FileFinder` 会被作为默认的 `path_hooks`, 你可以在 `cpython/Lib/importlib/_bootstrap_external.py` 中找到下面这段代码

```python3
def _install(_bootstrap_module):
    """Install the path-based import components."""
    _setup(_bootstrap_module)
    supported_loaders = _get_supported_file_loaders()
    sys.path_hooks.extend([FileFinder.path_hook(*supported_loaders)])
    sys.meta_path.append(PathFinder)

```

`FileFinder.find_spec`  会处理搜索过程, 和操作系统的文件系统交互, 并且把这些文件状态缓存下来, 当文件变更时通过缓存中的状态是可以被感知到的

当你想定义你自己的 `Finder` 时, 或者更改系统默认的 import 的行为, 你也可以更改 `sys.path_hooks` 这个变量