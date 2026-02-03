# memory management

* you may need more than 20 minutes to read this article

# contents

* [related file](#related-file)
* [introduction](#introduction)
	* [object allocator](#object-allocator)
	* [raw memory allocator](#raw-memory-allocator)
	* [block](#block)
	* [pool](#pool)
	* [arena](#arena)
	* [memory layout](#memory-layout)
	* [usedpools](#usedpools)
		* [why only half of the usedpools elements are used](#why-only-half-of-the-usedpools-elements-are-used)
* [example](#example)
	* [overview](#overview)
	* [how does memory block organize in pool](#how-does-memory-block-organize-in-pool)
		* [allocate in pool with no freed block](#allocate-in-pool-with-no-freed-block)
		* [free in pool](#free-in-pool)
			* [free in full pool](#free-in-full-pool)
			* [free in not full pool](#free-in-not-full-pool)
		* [allocate in pool with freed block](#allocate-in-pool-with-freed-block)
	* [how does pool organize in arena](#how-does-pool-organize-in-arena)
		* [arena overview part1](#arena-overview-part1)
		* [allocate in arena with no freed pool](#allocate-in-arena-with-no-freed-pool)
		* [free in arena](#free-in-arena)
		* [allocate in arena with freed pool](#allocate-in-arena-with-freed-pool)
		* [arena overview part2](#arena-overview-part2)
* [read more](#read-more)

# related file

* cpython/Modules/gcmodule.c
* cpython/Objects/object.c
* cpython/Include/cpython/object.h
* cpython/Include/object.h
* cpython/Include/objimpl.h
* cpython/Objects/obmalloc.c
* cpython/Python/pyarena.c

# introduction

CPython has implemented its own memory management mechanism. When you create a new object in a Python program, it's not directly malloced from the heap

![level](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/level.png)

We will figure out how the **Python interpreter** part works internally in the following example

## object allocator

This is the creation procedure of a `tuple` object with size n

Step 1: Check if there's a chance to reuse a `tuple` object in [free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#free-list). If so, go to step 4.

Step 2: Call **PyObject_GC_NewVar** to get a `PyTupleObject` with size `n` from the memory management system (if the size of the object is fixed, **_PyObject_GC_New** will be called instead).

Step 3: Call **_PyObject_GC_TRACK** to link the newly created object to the doubly linked list in [gc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md#track).**generation0**

Step 4: Return

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/tuple_new.png)

The maximum size of memory you are able to allocate is limited in **_PyObject_GC_Alloc**

```c
typedef ssize_t         Py_ssize_t;
#define PY_SSIZE_T_MAX ((Py_ssize_t)(((size_t)-1)>>1))
# PY_SSIZE_T_MAX is 8388608 TB in my machine, usually you don't need to worry about the limit
if (basicsize > PY_SSIZE_T_MAX - sizeof(PyGC_Head))
    return PyErr_NoMemory();

```

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/malloc.png)

## raw memory allocator

Following the call stack, we can find that the **raw memory allocator** is mostly defined in `cpython/Objects/obmalloc.c`

```c
#define SMALL_REQUEST_THRESHOLD 512

```

The procedure is described below:

* If the requested memory block is greater than **SMALL_REQUEST_THRESHOLD** (512 bytes), the request will be delegated to the operating system's allocator.
* Otherwise, the request will be delegated to Python's **raw memory allocator**.

## block

We need to know some concepts before we look into how Python's memory allocator works.

**block** is the smallest unit in Python's memory management system. The size of a block is the same as a single **byte**

```c
typedef uint8_t block

```

There are lots of memory **blocks** of different sizes. The word **block** in memory **block** has a different meaning from the type **block**; we will see this later

## pool

A **pool** stores a collection of memory **blocks** of the same size.

Usually, the total size of memory blocks in a pool is 4KB, which is the same as most system page sizes.

Initially, the addresses for different memory blocks in the same pool are contiguous

![pool](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool.png)

## arena

An **arena** stores 256KB malloced from the heap. It divides the memory block into 4KB per unit.

Whenever a new pool is needed, the allocator will ask an **arena** for a new memory block with size 4KB and initialize the memory block as a new **pool**

![arena](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena.png)

## memory layout

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/layout.png)

## usedpools

There's a C array named **usedpools** that plays an important role in the memory management mechanism.

The definition of **usedpools** in C is convoluted. The following picture shows how these mentioned objects are organized

![usedpools](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/usedpools.png)

Elements in **usedpools** are of type `pool_header *`. The size of **usedpools** is 128, but only half of the elements are in use.

**usedpools** stores **pool** objects with free blocks. Every **pool** object in **usedpools** has at least one free memory block.

If you get **pool 1** from **idx0**, you can get one memory block (8 bytes) from **pool 1** each time. If you get **pool 4** from **idx2**, you can get one memory block (24 bytes) from **pool 4** each time, and so on

```python3
 cpython/Objects/obmalloc.c
 * For small requests we have the following table:
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

```

**idx0** is the head of a double linked list, each element in the double linked list is a pointer to a **pool**, all pools in **idx0** will handle those memory request <= 8 bytes, no matter how many bytes caller request, **pool** in **idx0** will only return a memory block of size 8 bytes each time

**idx1** is the head of a double linked list, all pools in **idx1** will handle those memory request (9 bytes <= request_size <= 16 bytes), no matter how many bytes caller request, **pool** in **idx1** will only return a memory block of size 16 bytes each time

and so on

### why only half of the usedpools elements are used

Every memory request will be routed to the **idxn** in **usedpools**. There must be a very fast way to access **idxn** for the underlying memory request with size **nbytes**

```c
#define ALIGNMENT_SHIFT         3
size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT
# idxn = size + size
pool = usedpools[size + size]

```

if the request size **nbytes** is 7, (7 - 1) >> 3 is 0, idxn = 0 + 0, usedpools[0 + 0] will be the target list, so the head of **idx0** is the target pool

if the request size **nbytes** is 24, (24 - 1) >> 3 is 2, idxn = 2 + 2, usedpools[2 + 2] will be the target list, so the head of **idx2** is the target pool

The `usedpools`'s size is two times larger than what it actually uses, so that you are able to calculate the **idx** and access the first free pool in a very short time

# example

## overview

Assume we are going to request 5 bytes from Python's memory allocator. Because the request size is less than **SMALL_REQUEST_THRESHOLD** (512 bytes), it's routed to Python's raw memory allocator instead of the system's allocator (**malloc** system call)

```c
size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT = (5 - 1) >> 3 = 0
pool = usedpools[0 + 0]

```

So the **pool** header will be the first element in **idx0**. Following the linked list, we can find that the first **pool** which can offer memory blocks is **pool1**

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/example0.png)

## how does memory block organize in pool

Assume this is the current state of **pool1**

**ref.count** is a counter for how many **blocks** are currently in use

**freeblock** points to the next **block** free to use

**nextpool** and **prevpool** are used as chain of the double linked list in **usepools**

**arenaindex** indicates which **arena** current **pool** belongs to

**szidx** indicates which size class current pool belongs to, it's same as the **idx** number in **usepools**

**nextoffset** is the offset for the next free block

**maxnextoffset** act as the high water mark for **nextoffset**, if **nextoffset** exceeds **maxnextoffset**, it means all blocks in the pool are in used, and the pool is full

The total size of a **pool** is 4KB, same as the system page size

![pool_organize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize0.png)

### allocate in pool with no freed block

"No freed block" doesn't mean the pool is full and has no other room left. Instead, it means no block allocated in the pool has ever called **free**.

When we allocate a memory block <= 8 bytes, the block that **freeblock** points to will be chosen

![pool_organize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize1.png)

Because the value stored in the block that **freeblock** points to is a NULL pointer, there's no other **freed** block in the middle of the **pool**. We must get a block from the tail of the pool; these blocks from the tail have never been used before in the pool

**nextoffset** and **maxnextoffset** will be used to check whether there's room in the tail of the **pool**

**ref.count** will be incremented

**freeblock** points to the next free block

**nextoffset** will also be incremented size of one block

![pool_organize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize2.png)

Request one more memory block

**nextoffset** now exceeds **maxnextoffset**

![pool_organize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize3.png)

Request one more memory block

![pool_organize4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize4.png)

Because **nextoffset** is greater than **maxnextoffset**, the **pool** is full. There are no more freeblocks to use, so **freeblock** becomes a null pointer.

We need to unlink the pool from the **usedpools**.

![pool_organize_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_full.png)

In a higher-level view

![arena_pool_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_pool_full.png)

### free in pool

#### free in full pool

It's ideal when the used blocks and free blocks are separated by a **freeblock** pointer in a **pool** and every memory request is routed to the beginning of the separated pointer.

What if there are some used blocks in random locations of the pool being freed?

Assume we are freeing the block at address `0x10ae3dfe8` in **pool1**

![pool_organize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free0.png)

Step 1: Set the value in the **block** to the same value as **freeblock**

![pool_organize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free1.png)

Step 2: Make **freeblock** point to the current freeing **block**

![pool_organize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free2.png)

Decrement the value in **ref.count**.

Step 3: Check whether the **pool** is full (check whether the value in the freeing **block** is a NULL pointer). If so, relink the **pool** to **usedpools** and return; otherwise go to step 4

![pool_organize_free3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free3.png)

Step 4:
* Check if all the **pools** in the current **arena** are free. If so, free the entire **arena**.
* If this is the only free pool in the arena, add the arena back to the `usable_arenas` list.
* Sort **usable_arenas** to make sure the **arena** with the smaller count of free pools is in front

#### free in not full pool

Let's free the final block at address `0x10ae3dff8` in **pool1**.

The steps are the same as above.

Step 1

![pool_organize_free4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free4.png)

Step 2

![pool_organize_free5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free5.png)

Decrement the value in **ref.count**.

Step 3: The **pool** is not full, so go to step 4.

Step 4: Return

![pool_organize_free6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free6.png)

Let's free the second-to-last block at address `0x10ae3dff0` in **pool1**.

Step 1

![pool_organize_free7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free7.png)

Step 2

![pool_organize_free8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free8.png)

Decrement the value in **ref.count**.

Step 3: **pool1** is not full, so go to step 4.

Step 4: Return

![pool_organize_free9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free9.png)

### allocate in pool with freed block

Now there are some blocks that were previously freed in **pool1**.

Let's request one more memory block.

Because the value stored in the block that **freeblock** points to is not a NULL pointer, it's a pointer to another block in the same **pool**. This means this block was used before and then freed

![pool_organize_allocate_after_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free0.png)

We increment the **ref.count**.

Make **freeblock** point to what's stored in the current block.

The careful reader will realize that **freeblock** is actually a pointer to a singly linked list. The linked list will chain all the freed **blocks** in the same **pool** together.

If the singly linked list reaches the end, it means there are no more blocks that were used previously and freed. We should try to get a block from those never-used rooms in the **pool**. We already learned this strategy from [allocate in pool with no freed block](#allocate-in-pool-with-no-freed-block)

![pool_organize_allocate_after_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free1.png)

Request one more memory block


**freeblock** follows to the next position in the single linked list

![pool_organize_allocate_after_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free2.png)

If we request one more memory block, the **pool** will be full and unlinked from the **usedpools**. We've learned this before in [free in pool](#free-in-pool)

### how does pool organize in arena

#### arena overview part1

The **arena** stores a collection of **pools**.

This is the initial state.

The **usedpools** is empty and there are no alive **arena** objects

![arena_orgnaize_overview0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview0.png)

When we request a **block** with 10 bytes, 16 **arenas** will be created. Each **arena** will contain 64 **pools**, and each **pool** will occupy 4KB

![arena_orgnaize_overview1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview1.png)

The first free **pool** in the first available **arena** will be inserted into **idx1**

![arena_orgnaize_overview2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview2.png)

And the **idx1** of **usedpools** is not empty anymore. Following the procedure [allocate in pool with no freed block](#allocate-in-pool-with-no-freed-block), we can get the **block** from **pool1**

#### allocate in arena with no freed pool

"No freed pool" doesn't mean the arena is full and has no other room left. Instead, it means no pool allocated in the arena has ever called **free**.

Assume this is the current state of the first arena

![arena_orgnaize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize0.png)

Request one more **pool**.

Because **freepools** is a null pointer and **nfreepools** is greater than 0, the address that **pool_address** points to will be used as the new **pool**

![arena_orgnaize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize1.png)

**nfreepools** is decremented and **pool_address** will point to the next free **pool**.

![arena_orgnaize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize2.png)

Request one more **pool**. The **arena** is full

![arena_orgnaize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize3.png)

#### free in arena

When all the **blocks** in a pool are freed, the **pool** will also be unlinked from **usedpools** and inserted at the front of the **arena**.

Assume we are freeing **pool3**

**nextpool** in **pool3** will be set to the same value as **freepools** in **arena**

![arena_orgnaize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free0.png)

The value of **freepools** in **arena** will be set to the address of **pool3**

**nfreepools** will be incremented

![arena_orgnaize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free1.png)

Let's free **pool63**

**nextpool** of the current freeing **pool** will be set to the value in **freepools** and **freepools** in **arena** will point to the current freeing **pool**

**nfreepools** will be incremented

![arena_orgnaize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free2.png)

The free strategy of **arena** is similar to **pool**. **freepools** is the pointer to the head of the singly linked list

#### allocate in arena with freed pool

Request one more **pool**.

**nfreepools** is greater than 0, and **freepools** is not a NULL pointer, so the first **pool** in the singly linked list will be taken

![arena_orgnaize_allocate0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate0.png)

**nfreepools** will be decremented

![arena_orgnaize_allocate2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate2.png)

#### arena overview part2

Initially, there are 16 **arenas**

![arena_orgnize_overview_part20](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part20.png)

If all the **pools** in the **arena** are freed, the memory (256KB) these **pools** used will be returned to the operating system, and the **arena** structure will be moved to a singly linked list named **unused_arena_objects**. What's in **unused_arena_objects** is only a shell; it does not contain any **pool** free to use.

Prior to Python 2.5, **pools** in arenas were never free()'ed. This strategy has been used since Python 2.5

![arena_orgnize_overview_part21](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part21.png)

If all **arenas** are used up and we request a new **arena**

![arena_orgnize_overview_part22](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part22.png)

Python's allocator will malloc 256KB from the operating system, and the newly malloced 256KB memory block will be linked to the **arena** structure in the first **unused_arena_objects**.

The first **arena** in **unused_arena_objects** will be taken, and **unused_arena_objects** will become an empty pointer

![arena_orgnize_overview_part23](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part23.png)

If all **arenas** are used up and we request a new **arena** again when **unused_arena_objects** is empty

![arena_orgnize_overview_part24](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part24.png)

The size of the **arenas** will be doubled. When you need to request a new **pool** from **arena**, the next **arena** (position 17) will be chosen

![arena_orgnize_overview_part25](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnize_overview_part25.png)

You can call `sys._debugmallocstats()` to get some statistics about the memory management

# read more
* [Memory management in Python](https://rushter.com/blog/python-memory-managment/)