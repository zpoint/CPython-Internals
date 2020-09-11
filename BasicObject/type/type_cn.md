# type![image title](http://www.zpoint.xyz:8080/count/tag.svg?url=github%2FCPython-Internals/type_cn)

# 目录

* [相关位置文件](#相关位置文件)
* [mro](#mro)
	* [python2.3 以前](#python2.3-以前)
	* [python2.3 以后](#python2.3-以后)
	* [python2 和 python3 的区别](#python2-和-python3-的区别)
* [class 的创建](#class-的创建)
* [instance 的创建](#instance-的创建)
* [metaclass](#metaclass)
* [相关阅读](#相关阅读)

# 相关位置文件
* cpython/Objects/typeobject.c
* cpython/Objects/clinic/typeobject.c.h
* cpython/Include/cpython/object.h
* cpython/Python/bltinmodule.c


# mro

MRO(Method Resolution Order) 这个名词直接翻译有点绕口, 简单理解就是在面向对象继承多个类的时候, 决定了一个属性引用到哪个类去搜索的顺序

在 python2.3 之后, python 解释器中的 MRO 匹配实现了 [C3 superclass linearization](https://en.wikipedia.org/wiki/C3_linearization) 这个算法, 在 python2.3 之前, MRO 的算法策略是 深度优先, 从左到右的的算法

对MRO 感兴趣的同学可以参考这个油管视频 [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)

## python2.3 以前

我们定义一个示例看看

```python3
from __future__ import print_function

class A(object):
    pass

class B(object):
    def who(self):
        print("I am B")

class C(object):
    pass

class D(A, B):
    pass

class E(B, C):
    def who(self):
        print("I am E")

class F(D, E):
    pass

```

![mro_ hierarchy](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/mro_hierarchy.png)

python2.3 之前的 MRO 策略是深度优先, 从左往右

**D** 的 MRO 计算结果 `DAB`

**E** 的 MRO 计算结果 `EBC`

**F** 的 MRO 计算结果 `FDABEC`

```python3
# ./python2.2
>>> f = F()
>>> f.who() # B 在 E 前面
I am B

```

## python2.3 以后

**C3** 算法的 **merge** 部分可以简单概括为如下几个步骤

```python3
# 假设当前是 [list1, list2, list3, ... N]
# 示例: [A, BC, DC, ..., EFG]
# 每个 list 都可以分成头部, 尾部
# 示例中的 A 头部为 A, 尾部为 空
# 示例中的 BC 头部为 B, 尾部为 C
# 示例中的 EFG 头部为 E, 尾部为 FG

```

* 取出第一个 list 的头部元素
* 如果这个头部元素不在任何其他 list 的尾部中, 把这个头部元素加到结果中, 并从所有其他 list 中剔除这个头部元素
* 如果这个元素在其他 list 的尾部中, 从下一个 list 开始遍历, 直到找到第一个符合要求的头部元素位置
* 重复上面的过程, 直到所有的 list 都消失为止 (这里就需要你的继承顺序是单调的, 如继承顺序不能保证单调性, 这个算法是无法计算出结果的, 结尾的视频链接中有对应示例)

**D** 的 MRO 计算结果如下

```python3
D + merge(L(A), L(B), AB)
D + merge(A, B, AB)
# A 是下一个 list 的头部, 并且不在任何其他 list 的尾部中, 取出 A, 并把 A 从其他的所有的 list 中移除
D + A + merge(B, B)
# list 中的第一个元素应该当成头部元素而不是尾部元素, 所以 B 符合要求
D + A + B

```

**E** 的 MRO 计算结果如下

```python3
E + merge(L(B), L(C), BC)
E + merge(B, C, BC)
E + B + merge(C, C)
E + B + C

```

**F** 的 MRO 计算结果如下

```python3
F + merge(L(D), L(E), BC)
F + merge(DAB, EBC, DE)
F + D + merge(AB, EBC, E)
F + D + A + merge(B, EBC, E)
# 现在 B 在第二个 list 的尾部中, 第二个 list 的头部元素是 "E"
# 尾部元素是 "BC", 这个尾部存在了 B, 所以 B 不符合要求, 从下一个 list 的头部 "E" 开始匹配
F + D + A + E + merge(B, BC)
F + D + A + E + B + C

# ./python2.3
>>> f = F()
>>> f.who() # E 现在在 B 的前面
I am E

```

## python2 和 python3 的区别

在如下示例代码中

```python3
from __future__ import print_function

class A:
    pass

class B:
    def who(self):
        print("I am B")

class C:
    pass

class D(A, B):
    pass

class E(B, C):
    def who(self):
        print("I am E")

class F(D, E):
    pass

```

在 python2 中, class `A`, `B`,`C`, `D`, 和 `E` 并不是显式的继承自 `object`, 即使解释器版本在 python2.3 之后也是一样的, 不是显式的继承自 `object` 的对象不会用 **C3** 算法来处理 MRO, 而是保持原有的深度优先, 从左到右的算法

```python3
# ./python2.7
>>> f = F()
>>> f.who() # B 在 E 前
I am B

```

然而在 python3 中, 即使对象在定义时不是显式的继承自 `object`, 他们实际上处理的时候也会自动的继承自 `object`, 所有继承自 `object` 的对象会遵循 **C3** 算法进行 **MRO**

```python3
# ./python3
>>> f = F()
>>> f.who() # E 在 B 前
I am E

```

# class 的创建

如果我们用 `dis` 模块处理上述代码

```python3
 26          96 LOAD_BUILD_CLASS
             98 LOAD_CONST              12 (<code object F at 0x10edf4150, file "mro.py", line 26>)
            100 LOAD_CONST              13 ('F')
            102 MAKE_FUNCTION            0
            104 LOAD_CONST              13 ('F')
            106 LOAD_NAME                6 (D)
            108 LOAD_NAME                7 (E)
            110 CALL_FUNCTION            4
            112 STORE_NAME               8 (F)

```

`LOAD_BUILD_CLASS` 只是把 `__build_class__` 压入 stack 中

接下来的 opcodes 把其他 `__build_class__` 需要的参数也压入 stack 中

`110 CALL_FUNCTION            4` 调用了 `__build_class__` 这个函数生成了对应的 `class`

`__build_class__`  会找到对应的 `metaclass`, `name`, `bases`, 和 `ns`(namespace) 并调用 `metaclass.__call__` 来生成最后的 `class`

比如 `class F`

```python3
metaclass: <class 'type'>
name: 'F'
bases: (<class '__main__.D'>, <class '__main__.E'>)
ns: {'__module__': '__main__', '__qualname__': 'F'}, cls: <class '__main__.F'>

```

在示例 `class F` 中, `<class 'type'>.__call__` 做了什么呢 ?

这个函数定义的位置在 `cpython/Objects/typeobject.c`

函数原型是 `static PyObject *type_call(PyTypeObject *type, PyObject *args, PyObject *kwds)`

我们可以根据上面位置的代码画出如下流程

![creation_of_class](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_class.png)

# instance 的创建

如果我们在上面示例代码的尾部增加一行

```python3
f = F()

```

我们可以在 `dis` 中找到另外一段片段

```python3

 31         130 LOAD_NAME                9 (F)
            132 CALL_FUNCTION            0
            134 STORE_NAME              10 (f)

```

它只调用了 `F` 的 `__call__` 属性

```python3

>>> F.__call__
<method-wrapper '__call__' of type object at 0x7fa5fa725ec8>

```

![creation_of_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/creation_of_instance.png)

# metaclass

在上面的过程中, 我们可以发现, **metaclass** 控制了 class 的创建, instance(实例) 的创建过程并没有 **metaclass** 什么事, 在创建实例的过程中仅仅是调用了 class 的 `__call__` 属性去生成一个实例

![difference_between_class_instance](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/type/difference_between_class_instance.png)

根据以上发现, 我们可以自己定义一个 **metaclass** 在代码运行过程中个性化定制最终的 class

```python3
class F(object):
    pass


class MyMeta(type):
    def __new__(mcs, name, bases, attrs, **kwargs):
        if F in bases:
            return F

        newly_created_cls = super().__new__(mcs, name, bases, attrs)
        if "Animal" in name:
            newly_created_cls.leg = 4
        else:
            newly_created_cls.leg = 0
        return newly_created_cls


class Animal1(metaclass=MyMeta):
    pass


class Normal(metaclass=MyMeta):
    pass


class AnotherF(F, metaclass=MyMeta):
    pass


print(Animal1.leg)  # 4
print(Normal.leg)  # 0
print(AnotherF)  # <class '__main__.F'>

```

挺有趣的对嘛!

# 相关阅读
* [Amit Kumar: Demystifying Python Method Resolution Order - PyCon APAC 2016](https://www.youtube.com/watch?v=cuonAMJjHow&t=400s)
* [The Python 2.3 Method Resolution Order(MRO)](https://www.python.org/download/releases/2.3/mro/)
* [understanding-python-metaclasses](https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses)