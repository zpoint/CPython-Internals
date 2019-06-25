# memory management

* 长文, 预计阅读时间在二十分钟以上

# 目录

* [相关位置文件](#相关位置文件)
* [介绍](#介绍)
	* [object allocator](#object-allocator)
	* [raw memory allocator](#raw-memory-allocator)
	* [block](#block)
	* [pool](#pool)
	* [arena](#arena)
	* [内存构造](#内存构造)
	* [usedpools](#usedpools)
		* [为什么 usedpools 中只用到一半的元素](#为什么-usedpools-中只用到一半的元素)
* [示例](#示例)
	* [概述](#概述)
	* [block 是如何在 pool 中组织的](#block-是如何在-pool-中组织的)
		* [在未释放过 block 的 pool 中申请新的空间](#在未释放过-block-的-pool-中申请新的空间)
		* [在 pool 中释放空间](#在-pool-中释放空间)
			* [在满的 pool 中进行释放](#在满的-pool-中进行释放)
			* [在有空余空间的 pool 中进行释放](#在有空余空间的-pool-中进行释放)
		* [在释放过 block 的 pool 中申请新的空间](#在释放过-block-的-pool-中申请新的空间)
	* [pool 是如何在 arena 中组织的](#pool-是如何在-arena-中组织的)
		* [arena 概述1](#arena-概述1)
		* [在未释放过 pool 的 arena 中申请新的空间](#在未释放过-pool-的-arena-中申请新的空间)
		* [在 arena 中释放空间](#在-arena-中释放空间)
		* [在释放过 pool 的 arena 中申请新的空间](#在释放过-pool-的-arena-中申请新的空间)
		* [arena 概述2](#arena-概述2)
* [相关阅读](#相关阅读)

# 相关位置文件

* cpython/Modules/gcmodule.c
* cpython/Objects/object.c
* cpython/Include/cpython/object.h
* cpython/Include/object.h
* cpython/Include/objimpl.h
* cpython/Objects/obmalloc.c

# 介绍

CPython 实现了自己的内存管理机制, 当你在 python 程序中创建一个新的对象时, 并不是从进程的 heap 空间中直接申请一块对应大小的空间的

![level](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/level.png)

我们会通过以下的示例来了解 **python interpreter** 部分的工作原理

## object allocator

这是一个创建大小为 n 的 `tuple` 对象的过程

step1, 检查是否能使用 `tuple` 对象中对应 [缓冲池](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple_cn.md#free-list) 中的空间, 如果可以进到 step4

step2, 调用 **PyObject_GC_NewVar** 从 CPython 内存管理系统中获取一个大小为 `n` 的 `PyTupleObject` 对象(如果不是大小为 `n`, 而是其他大小固定的对象, 调用的会是 **_PyObject_GC_New** 函数)

step3, 调用 **_PyObject_GC_TRACK** 把这个新创建的 `PyTupleObject` 对象加到 [垃圾回收机制](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc_cn.md#track) 的 **generation0** 中

step4, 返回给调用者

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/tuple_new.png)

你理论上能申请的最大的空间在 **_PyObject_GC_Alloc** 中做了限制

	typedef ssize_t         Py_ssize_t;
	#define PY_SSIZE_T_MAX ((Py_ssize_t)(((size_t)-1)>>1))
    # PY_SSIZE_T_MAX 在我的机器上是一个 8 字节的有符号类型,  能最大表示 8388608 TB 的大小, 所以通常我们不需要担心在这一步超过了限制
    if (basicsize > PY_SSIZE_T_MAX - sizeof(PyGC_Head))
        return PyErr_NoMemory();

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/malloc.png)

## raw memory allocator

通过搜索调用栈, 我们可以发现 **raw memory allocator** 大部分都是在 `cpython/Objects/obmalloc.c` 中定义的

	#define SMALL_REQUEST_THRESHOLD 512

整个过程大致如下

* 如果申请的内存空间超过 **SMALL_REQUEST_THRESHOLD**(512 bytes), 这个申请会转发给默认的 malloc 函数, 交给操作系统从 heap 中申请
* 如果申请的内存空间不超过 **SMALL_REQUEST_THRESHOLD**(512 bytes), 这个申请会转发给 python 实现的 **raw memory allocator**

## block

在了解 python 的 memory allocator 如何工作之前, 我们需要对一些相关的名称有一个初步的了解

**block** 在 python 的内存管理系统中作为一个最小的单元, 一个 **block** 的大小和一个 **byte** 的大小是相同的

	typedef uint8_t block

在我这里, 内存块表示一段连续的空间(比如 24 bytes 大小的连续空间), 而 **block**(块) 表示上面定义的最基本单位, 后面会有更多示例

## pool

一个 **pool** 存储了许多的大小相同的内存块

通常来讲, 一个 **pool** 的大小(包括所有内存块的和) 为 4kb, 和大部分操作系统的 页 大小相同

在最初的时候, 一个 **pool** 中存储的不同内存块的地址是连续的

![pool](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool.png)

## arena

一个 **arena** 对象上会存储256kb 大小的从 heap 分配的空间, 它把这些大小平均分成 64 份, 每份 4 kb

每当调用者需要申请空间, 并且当前 **pool** 已经用完, 需要获取一个新的 **pool** 的时候, allocator 会向下一个 **arena** 申请一块大小为 4kb 的内存, 并且把这一块内存空间初始化为一个新的 **pool**

![arena](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena.png)

## 内存构造

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/layout.png)

## usedpools

存在一个全局的 C 数组, 数组名称为 **usedpools**, **usedpools** 在内存管理机制中起了非常重要的作用

**usedpools** 在 C 中的定义和作用比较晦涩, 下图大概展示了上面提到的对象是如何组织串联在一起的

![usedpools](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/usedpools.png)

**usedpools** 中的元素类型为 `pool_header *`, **usedpools** 的大小为 128, 但是实际上只有一半的元素是用到了的

**usedpools** 存储了含有空闲空间的 **pool** 对象, **usedpools** 中的每一个 **pool** 至少含有一个未使用的内存块

如果你从 **idx0** 中获取到了 **pool 1**, 那么从 **pool 1** 中你每次能获得一个内存块(8 bytes)大小的空余空间, 如果你从 **idx2** 中获取到了 **pool 4**, 那么从 **pool 4** 中你每次能获得一个内存块(24 bytes)大小的空余空间, 以此类推


	 cpython/Objects/obmalloc.c
     * 对于小块内存空间的申请, 我们有如下的表
     *
     * Request in bytes     Size of allocated block      Size class idx
     * ----------------------------------------------------------------
     *        1-8                     8                       0
     *        9-16                   16                       1
     *       17-24                   24                       2
     *       25-32                   32                       3
     *       33-40                   40                       4
     *       41-48                   48                       5
     *       49-56                   56                       6
     *       57-64                   64                       7
     *       65-72                   72                       8
     *        ...                   ...                     ...
     *      497-504                 504                      62
     *      505-512                 512                      63
     *
     *      0, SMALL_REQUEST_THRESHOLD + 1 and up: routed to the underlying
     *      allocator.
     */

**idx0** 是一个双端链表的表头的入口, 链表上的每一个元素都指向一个 **pool** 对象, **idx0** 中的所有的 **pool** 会处理那些 <= 8 bytes 的申请内存的请求, 无论调用者需要多少空间(1 bytes, 3bytes 还是 6bytes 都一样), **idx0** 中的 **pool** 对象每次都只会返回一个 8 bytes 的对象供申请者使用

**idx1** 也是一个双端链表的表头的入口,  **idx1** 中的所有的 **pool** 会处理那些 9 bytes <= request_size <= 16 bytes 的申请内存的请求, 无论调用者需要多少空间(9 bytes, 12bytes 还是 15bytes 都一样), **idx1** 中的 **pool** 对象每次都只会返回一个 16 bytes 的对象供申请者使用

以此类推

### 为什么 usedpools 中只用到一半的元素

通常应用中小块的内存申请是比较频繁的, 每一次内存申请都需要找到 **usedpools** 中对应的 **idxn**, 所以对于每次内存分配申请大小为 **nbytes** 的请求, 必须有一种非常快就能定位到对应的 **idxn** 的方法

    #define ALIGNMENT_SHIFT         3
    size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT
    # idxn = size + size
    pool = usedpools[size + size]

如果申请的大小 **nbytes** 为 7, (7 - 1) >> 3 为 0,  idxn = 0 + 0, usedpools[0 + 0] 就是被选中的链表, **idx0** 中的第一个 **pool** 既为获取到的 **pool**

如果申请的大小 **nbytes** 为 24, (24 - 1) >> 3 为 2,  idxn = 2 + 2, usedpools[0 + 0] 就是被选中的链表, **idx2** 中的第一个 **pool** 既为获取到的 **pool**

`usedpools` 的大小为实际需要大小的两倍, 你可以通过以上的位移和加减运算就得到对应的 **idx** 位置, 这样设计极大地缩短了获得对应 **pool** 的入口的时间

# 示例

## 概述

假设我们需要通过 python 的 memory allocator 申请 5 bytes 大小的空间, 因为申请的大小比 **SMALL_REQUEST_THRESHOLD**(512 bytes) 小, 这个请求会被转发到 python 的 raw memory allocator 中, 而不是操作系统默认的 allocator(使用 **malloc** 这个系统调用)

	size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT = (5 - 1) >> 3 = 0
    pool = usedpools[0 + 0]

**idx0** 会被选中, 跟着这个链表, 我们可以找到第一个拥有空闲空间的 **pool**, 就是图中的 **pool1**

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/example0.png)

## block 是如何在 pool 中组织的

假设下图是 **pool1** 当前的状态

**ref.count** 是一个计数器, 表示有多少个 **block** 是已经被使用的

**freeblock** 指向了下一个空闲的 **block**

**nextpool** 和 **prevpool** 被用作 **usepools** 中双端链表的链条

**arenaindex** 表明了当前 **pool** 属于哪一个 **arena**

**szidx** 表明当前的 **pool** 分配哪一类的空间, 值和 **usepools** 中的 **idx** 值相同

**nextoffset** 是尾部未使用过的空间的位移

**maxnextoffset** 表示了 **nextoffset** 最高能超过的位置, 如果 **nextoffset** 超过了 **maxnextoffset**, 则表示 **pool** 中所有的 **block** 都已经使用过了, 如果这些 **block** 都没有释放过, 这个 **pool** 就是满的

**pool** 的大小为 4kb, 和操作系统的页大小一致

![pool_organize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize0.png)

### 在未释放过 block 的 pool 中申请新的空间

未释放过 **block** 的意思是 **pool** 中的 **block** 被申请过后没有进行释放

当我们申请一个 <= 8 bytes 的内存块时

**freeblock** 指向的 **block** 会被选中

![pool_organize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize1.png)

因为 **freeblock** 指向的是一个 NULL 指针, 表示目前 **pool** 中没有其他之前被申请过但是当前已经被释放了的 **block** 存在, 新申请空间的话, 需要从 **pool** 尾部寻找之前没有用到过的新的空间

**nextoffset** 和 **maxnextoffset** 在这种情况下会被用来查看 **pool** 尾部是否还有剩余的未使用过的空间

**ref.count** 上的值会被增加

**freeblock** 指向下一个空闲的 **block**

**nextoffset** 也会被增加, 增加的值为当前 **pool** 中一个内存块的大小

![pool_organize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize2.png)

再申请一个新的内存块

**nextoffset** 大小现在超过了 **maxnextoffset**

![pool_organize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize3.png)

再申请一个新的内存块

![pool_organize4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize4.png)

因为 **nextoffset** 的值比 **maxnextoffset** 的大, 表示尾部已经没有多余的新的未使用过的空间可以使用了, 并且目前情况下当前的 **pool** 是满的了, 所以 **freeblock** 变成了一个空指针

因为 **pool** 已经满了, 我们需要把 **pool** 从 **usedpools** 中移除

![pool_organize_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_full.png)

在更高一层的角度看

![arena_pool_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_pool_full.png)

### 在 pool 中释放空间

#### 在满的 pool 中进行释放

一个 **pool** 中, 已使用的 **block** 和从未使用过的 **block** 在地址上被 **freeblock** 分隔开来, 并且每一次有新的内存空间的申请, 都从 这条分界线取下一块空间, 这样的设想是非常理想化的

如果在任意的一个位置, 已经被使用过的空间需要进行释放回收怎么办呢 ?

假设我们需要释放 **pool1** 上的地址为 `0x10ae3dfe8` 的这个内存块

![pool_organize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free0.png)

step1, 把内存块里第一个**block**(这里的内存块和**block**大小是相同的)的值设置为和 **freeblock** 相同的值

![pool_organize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free1.png)

step2, 让 **freeblock** 指向当前正在释放的 **block**

![pool_organize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free2.png)

减小 **ref.count** 的值

step3, 检查 **pool** 是否是满的(既检查当前正在释放的 **block** 是否空指针), 如果是, 把 **pool** 重新链接到 **usedpools** 中并返回, 如果不是则跳转到 step4

![pool_organize_free3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free3.png)

step4
* 检查当前 **arena** 中的所有的 **pools** 是否都为空的, 如果是的话, 释放整个 **arena**
* 如果这是 **arena** 中唯一的有空余空间的 **pool**, 把 **arena** 加回到 `usable_arenas` 列表中
* 给 **usable_arenas** 进行排序, 确保更多空的 **pool** 的 **arena** 排在更前面

#### 在有空余空间的 pool 中进行释放

我们来释放 **pool1** 的最后一块内存块, 地址为 `0x10ae3dff8`

下面的步骤和上面相同, 就不详细描述了

step1

![pool_organize_free4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free4.png)

step2

![pool_organize_free5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free5.png)

减小 **ref.count** 的值

step3, **pool** 不是满的, 跳转到 step4

step4, 返回

![pool_organize_free6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free6.png)

我们来释放 **pool1** 中的倒数第二个内存块, 地址为 `0x10ae3dff0`

step1

![pool_organize_free7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free7.png)

step2

![pool_organize_free8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free8.png)

减小 **ref.count** 的值

step3, **pool1** 不是满的, 跳转到 step4

step4, 返回

![pool_organize_free9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free9.png)

### 在释放过 block 的 pool 中申请新的空间

现在 **pool1** 中有一些调用过 **free** 的内存块了

我们来申请一个新的内存块

因为存储在 **freeblock** 中的值并不是一个 NULL 指针, 它是一个指向相同的 **pool** 中的其他内存块的指针, 表示这个被指向的内存块之前被使用过, 但是当前已经被释放了

![pool_organize_allocate_after_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free0.png)

增加 **ref.count** 的值

让 **freeblock** 指向的地址和当前内存块存储的地址相同

细心地读者已经意识到了 **freeblock** 实际上是一个指向一个单链表的指针, 这个单链表会把所有的同个 **pool** 中申请过之后释放了的内存块串联起来

如果这个单链表到了底部, 表示当前 **pool** 中没有更多之前申请过之后释放了的内存块可以使用了, 我们只能尝试从尾部获取从未使用过的内存空间, 之前已经在 [在未释放过 block 的 pool 中申请新的空间](#在未释放过-block-的-pool-中申请新的空间) 部分学习过了

![pool_organize_allocate_after_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free1.png)

申请一个新的内存块

**freeblock** 跟着这个单链表到达下一个位置

![pool_organize_allocate_after_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free2.png)

如果我们再申请一个新的内存块, **pool** 会变满, 并且从 **usedpools** 中去除, 这一部分在 [在 pool 中释放空间](#在-pool-中释放空间) 前面一点的位置也学习过了

### pool 是如何在 arena 中组织的

#### arena 概述1

一个 **arena** 存储了许多的大小相同的 **pool**

这是最初始的状态

**usedpools** 是空的, 并且没有能用的 **arena** 对象

![arena_orgnaize_overview0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview0.png)

假设我们申请一个 10 bytes 的内存块, 因为当前没有能用的 **pool** 和 **arena**, 16 个新的 **arena** 会被创建出来, 每一个 **arena** 会包含 64 个可用的 **pool**, 每个 **pool** 的大小为 4kb

![arena_orgnaize_overview1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview1.png)

第一个可用的 **arena** 中的第一个空闲的 **pool** 会被插入 **idx1**

![arena_orgnaize_overview2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview2.png)

**usedpools** 中的 **idx1** 的位置不再是空的链表了, 根据 [在未释放过 block 的 pool 中申请新的空间](#在未释放过-block-的-pool-中申请新的空间) 的过程我们可以从 **poo1** 中获得一个需要的内存块

#### 在未释放过 pool 的 arena 中申请新的空间

未释放过 **pool** 的意思是 **arena** 中的 **pool** 被申请过后没有进行释放

假设这是第一个 **arena** 当前的状态

![arena_orgnaize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize0.png)

申请一个新的 **pool**

因为 **freepools** 是一个空的指针, **nfreepools** 的值大于 0, **pool_address** 指向的地址会被取出作为新的 **pool**

![arena_orgnaize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize1.png)

**nfreepools** 的值被减小, 并且 **pool_address** 指向下一个空余的 **pool**

![arena_orgnaize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize2.png)

在申请一个新的 **pool**, **arena** 现在满了

![arena_orgnaize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize3.png)

#### 在 arena 中释放空间

当所有的 **pool** 中所有的 **block** 释放之后, **pool** 会从 **usedpools** 中移除, 并且插入到与之对应的 **arena** 前面

假设我们正在释放 **pool3**

**pool3** 中的 **nextpool** 会被设置为 **arena** 中和 **freepools** 相同的值

![arena_orgnaize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free0.png)

**arena** 中的 **freepools** 的值会被设置为 **pool3** 的地址

**nfreepools** 的值会被增加

![arena_orgnaize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free1.png)

我们来尝试释放 **pool63**

当前释放的 **pool** 中的 **nextpool** 值会被设置为和 **freepools** 相同的值, 并且 **freepools** 会指向当前释放的 **pool** 的地址

**nfreepools** 的值会被增加

![arena_orgnaize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free2.png)

在 **arena** 中进行释放的过程和在 **pool** 中进行释放的过程是相似的, **freepools** 指向一个单链表的表头

#### 在释放过 pool 的 arena 中申请新的空间

申请一个新的 **pool**

**nfreepools** 的值大于 0, 并且 **freepools** 并不是一个空指针, 这个单链表的第一个 **pool** 会被取出

![arena_orgnaize_allocate0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate0.png)

**nfreepools** 值会被减小

![arena_orgnaize_allocate2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate2.png)

#### arena 概述2

第一次分配 **arenas** 的时候, 只分配 16 个 **arenas**

![arena_orgnize_overview_part20](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part20.png)

如果 **arena** 中的所有的 **pool** 都被释放了, 那么这个 **arena** 会被移到一个名为 **unused_arena_objects** 的单链表下

并且这个 **arena** 中的所有的 **pools** 占用的空间(一共 256kb)会被释放, 但是 **arena** 这个 C 语言中的结构体不会被释放

在 Python 2.5 之前, **arena** 申请过的空间是从来不会被释放的, 这个策略是在 Python 2.5 之后引入的

![arena_orgnize_overview_part21](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part21.png)

如果所有的 **arenas** 都用完了, 下一次申请空间我们会需要一个新的 **arena**

![arena_orgnize_overview_part22](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part22.png)

python 的内存管理机制会重新像操作系统申请 256kb 的空闲空间, 并且这部分新申请的空间会挂到 **unused_arena_objects** 中的第一个 **arena** C 结构体中

**unused_arena_objects** 中的第一个 **arena** 会被返回, 并且此时 **unused_arena_objects** 会成为一个空指针

![arena_orgnize_overview_part23](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part23.png)

此时如果所有的 **arenas** 再次用完, 并且我们需要申请一个新的 **arena**, **unused_arena_objects** 也无可用的 **arena** 供我们使用

![arena_orgnize_overview_part24](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part24.png)

内存管理机制会向操作系统申请多一倍的 **arena**, 当你需要新的 **pool** 时, 下一个 **arena**(位置 17) 上面的 **pool** 会被返回

![arena_orgnize_overview_part25](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part25.png)

# 相关阅读

* [Memory management in Python](https://rushter.com/blog/python-memory-managment/)