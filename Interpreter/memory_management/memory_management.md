# memory management

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

# introduction

CPython has defined it's own memory management mechanism, when you create a new object in python program, it's not directly malloced from the heap

![level](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/level.png)

we will figure out how the **python interpreter** part works internally in the following example

## object allocator

this is the created procedure of a `tuple` object with size n

step1, check if there's a chance to reuse `tuple` object in [free_list](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/tuple/tuple.md#free-list), if so, goes to step4

step2, call **PyObject_GC_NewVar** to get a `PyTupleObject` with size `n` from memory management system(if the size of the object is fixed, **_PyObject_GC_New** will called instead)

step3, call **_PyObject_GC_TRACK** to link the newly created object to the double linked list in [gc](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/gc/gc.md#track).**generation0**

step4, return

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/tuple_new.png)

the maximum size of memory you are able to allocated is limited in **_PyObject_GC_Alloc**

	typedef ssize_t         Py_ssize_t;
	#define PY_SSIZE_T_MAX ((Py_ssize_t)(((size_t)-1)>>1))
    # PY_SSIZE_T_MAX is 8388608 TB in my machine, usually you don't need to worry about the limit
    if (basicsize > PY_SSIZE_T_MAX - sizeof(PyGC_Head))
        return PyErr_NoMemory();

![tuple_new](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/malloc.png)

## raw memory allocator

follow the call stack, we can find that the **raw memory allocator** is mostly defined in `cpython/Objects/obmalloc.c`

	#define SMALL_REQUEST_THRESHOLD 512

the procedure is described below

* if the request memory block is greater than **SMALL_REQUEST_THRESHOLD**(512 bytes), the request will be delegated to the operating system's allocator
* else, the request will be delegated to python's **raw memory allocator**

## block

we need to know some concept before we look into how python's memory allocator work

**block** is the smallest unit in python's memory management system, the size of a blok is the same size as a single **byte**

	typedef uint8_t block

there're lots of memory **block** in differenct size, word **block** in memory **block** has a different meaning from type **block**, we will see later

## pool

a **pool** stores a collection of memory **block** of the same size

usually, the total size of memory blocks in a pool is 4kb, which is the same as most of the system page size

initially, the addresses for different memory block in the same pool are continously

![pool](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool.png)

## arena

an **arena** stores 256kb malloced from heap, it divides the memory block to 4kb per unit

whenever there needs a new pool, the allocator will ask an **arena** for a new memory block with size 4kb, and initialize the memory block as a new **pool**

![arena](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena.png)

## memory layout

![layout](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/layout.png)

## usedpools

there's a C array named **usedpools** plays an important role in the memory management mechanism

the defination of **usedpools** in C is convoluted, the following picture shows how these mentioned objects organized

![usedpools](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/usedpools.png)

element in **usedpools** is of type `pool_header *`, size of **usedpools** is 128, but only half of the elements are in used

**usedpools** stores **pool** object with free blocks, every **pool** object in **usedpools** has at least one free memory block

if you get **pool 1** from **idx0**, you can get at least one memory block(8 bytes) from **pool 1**, if you get **pool 4** from **idx2**, you can get at least one memory block(24 bytes) from **pool 4**, and so on

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

**idx0** is the head of a double linked list, each element in the double linked list is a pointer to a **pool**, all pools in **idx0** will handle those memory request <= 8 bytes, no matter how many bytes caller request, **pool** in **idx0** will only return a memory block of size 8 bytes each time

**idx2** is the head of a double linked list, all pools in **idx0** will handle those memory request (9 bytes <= request_size <= 16 bytes), no matter how many bytes caller request, **pool** in **idx0** will only return a memory block of size 16 bytes each time

and so on

### why only half of the usedpools elements are used

every memory request will be routed to the **idxn** in **usedpools**, there must be a very fast way to access **idxn** for the underlying memory request with size **nbytes**

    #define ALIGNMENT_SHIFT         3
    size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT
    # idxn = size + size
    pool = usedpools[size + size]

if the request size **nbytes** is 7, (7 - 1) >> 3 is 0, idxn = 0 + 0, usedpools[0 + 0] will be the target list, so the head of **idx0** is the target pool

if the request size **nbytes** is 24, (24 - 1) >> 3 is 2, idxn = 2 + 2, usedpools[2 + 2] will be the target list, so the head of **idx2** is the target pool

the `usedpools`‘s size is two times larger than the size it actually used, so that you are able to calcuate the **idx** and access the first free pool in a very short time

# example

## overview

assume we are going to reqest 5 bytes from python's memory allocator, because the request size is less than **SMALL_REQUEST_THRESHOLD**(512 bytes), it's routed to python's raw memory allocator instead of the system's allocator(**malloc** system call)

	size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT = (5 - 1) >> 3 = 0
    pool = usedpools[0 + 0]

so **pool** header will be the first element in **idx0**, follow the linked list, we can find that the first **pool** which can offer memory blocks is **pool1**

![example0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/example0.png)

## how does memory block organize in pool

assume this is the current state of **pool1**

**ref.count** is a counter for how many **blocks** are currently in use

**freeblock** points to the next **block** free to use

**nextpool** and **prevpool** are used as chain of the double linked list in **usepools**

**arenaindex** indicates which **arena** current block belongs to

**szidx** indicates which size class current pool belongs to, it's same as the **idx** number in **usepools**

**nextoffset** is the offset for the next free block

**maxnextoffset** act as the high water mark for **nextoffset**, if **nextoffset** exceeds **maxnextoffset**, it means all blocks in the pool are in used, and the pool is full

the total size of a **pool** is 4kb, same as system page size

![pool_organize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize0.png)

### allocate in pool with no freed block

"no freed block" doesn't mean the pool is full and no any other room left, instead, it means no block allocated in the pool ever called **free**

when we allocate a memory block <= 8 bytes

the block **freeblock** pointed to will be chosen

![pool_organize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize1.png)

because the value stored in block **freeblock** pointed to is a NULL pointer, there's no other **freed** block in the middle of the **pool**, we must get a block from the tail of the pool, these blocks from the tail are never used before in the pool

**nextoffset** and **maxnextoffset** will be used to check whether there's room in the tail of the **pool**

**ref.count** will be incremented

**freeblock** points to the next free block

**nextoffset** will also be incremented size of one block

![pool_organize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize2.png)

request one more memory block

**nextoffset** now exceeds **maxnextoffset**

![pool_organize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize3.png)

request one more memory block

![pool_organize4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize4.png)

because the **nextoffset** is greater than **maxnextoffset**, the **pool** is full, there're no more freeblocks to use, so **freeblock** becomes a null pointer

we need to unlink the pool from the **usedpools**

![pool_organize_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_full.png)

in higher level view

![arena_pool_full](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_pool_full.png)

### free in pool

#### free in full pool

it's idealize that the used blocks and free blocks are seperated by a **freeblock** pointer in a **pool** and every memory request will be routed to the beginning of the seperated pointer

what if there're some used blocks in the random location of the pool being freed ?

assume we are freeing the block in address `0x10ae3dfe8` in **pool1**

![pool_organize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free0.png)

step1, set the value in the **block** to the same value as **freeblock**

![pool_organize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free1.png)

step2, make the **freeblock** points to the current freeing **block**

![pool_organize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free2.png)

decrement the value in **ref.count**

step3, check whether the **pool** is full(check whether the value in the freeing **block** is NULL pointer), if so, relink the **pool** to used pool and return, else go to step4

![pool_organize_free3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free3.png)

step4
* check if the all the **pools** in the current **arena** are free, if so free the entire **arena**
* if this is the only free pool in the arena, add the arena back to the `usable_arenas` list
* sort **usable_arenas** to make sure the **arena** with smaller count of free pools in the front

#### free in not full pool

let's free the final block in address `0x10ae3dff8` in **pool1**

the steps are same as above steps

step1

![pool_organize_free4](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free4.png)

step2

![pool_organize_free5](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free5.png)

decrement the value in **ref.count**

step3, the **pool** is not full, so goes to step4

step4, return

![pool_organize_free6](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free6.png)

let's free the second to the last block in address `0x10ae3dff0` in **pool1**

step1

![pool_organize_free7](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free7.png)

step2

![pool_organize_free8](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free8.png)

decrement the value in **ref.count**

step3, the **pool** is not full, so goes to step4

step4, return

![pool_organize_free9](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_free9.png)

### allocate in pool with freed block

now, there're some blocks called **free** previously in the **pool**

let's request one more memory block

because the value stored in block **freeblock** pointed to is a not a NULL pointer, it's a pointer to other block in the same **pool**, it means this block is used before and freed

![pool_organize_allocate_after_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free0.png)

we increment the **ref.count**

make the **freeblock** points to what's stored in the current block

the careful reader will realize that **freeblock** is actually a pointer to a single linked list, the linked list will chain all the freed **blocks** in the same **pool** together

if the single linked list reaches the end, it means there're no more blocks used previously and freed, we should try to get a block from those never used room before in the **pool**, we already learn this strategy from [allocate in pool with no freed block](#allocate-in-pool-with-no-freed-block)

![pool_organize_allocate_after_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free1.png)

request one more memory block


**freeblock** follows to the next position in the single linked list

![pool_organize_allocate_after_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/pool_organize_allocate_after_free2.png)

if we request one more memory block, the **pool** will be full and unlink from the **usedpools**, we've learned before [free in pool](#free-in-pool)

### how does pool organize in arena

#### arena overview part1

the **arena** stores a collection of **pool**

this is the initial state

the **usedpools** is empty and there're no alive **arena** object

![arena_orgnaize_overview0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview0.png)

when we request a **block** with 10 bytes, 16 **arena** will be created, each **arena** will contain 64 **pool**, each **pool** will occupy 4kb

![arena_orgnaize_overview1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview1.png)

the first free **pool** in the first avaliable **arena** will be inserted to the **idx1**

![arena_orgnaize_overview2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_overview2.png)

and the **idx1** of **usedpools** is not empty any more, follow the procedure [allocate in pool with no freed block](#allocate-in-pool-with-no-freed-block), we can get the **block** from **poo1**

#### allocate in arena with no freed pool

"no freed pool" doesn't mean the arena is full and no any other room left, instead, it means no pool allocated in the arena ever called **free**

assume this is the current state of the first arena

![arena_orgnaize0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize0.png)

request one more **pool**

because the **freepools** is a null pointer, and **nfreepools** is greater than 0, the address **pool_address** pointed to will be used as the new **pool**

![arena_orgnaize1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize1.png)

**nfreepools** is decremented and **pool_address** will point to next free **pool**

![arena_orgnaize2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize2.png)

request one more **pool**, the **arena** is full

![arena_orgnaize3](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize3.png)

#### free in arena

when all the **blocks** in a pool are freed, the **pool** will also be unlink in the **usedpools** and insert to the front of the **arena**

assume we are freeing **pool3**

**nextpool** in **pool3** will be set to the same value as **freepools** in **arena**

![arena_orgnaize_free0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free0.png)

value of **freepools** in **arena** will be set to the address of **pool3**

**nfreepools** will be incremented

![arena_orgnaize_free1](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free1.png)

let's free **pool63**

**nextpool** of the current freeing **pool** will be set to the value in **freepools** and **freepools** in **arena** will point to the current freeing **pool**

**nfreepools** will be incremented

![arena_orgnaize_free2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_free2.png)

the free strategy of **arena** is similar to **pool**, **freepools** is the pointer to the header of the single linked list

#### allocate in arena with freed pool

request one more **pool**

**nfreepools** is greater than 0, and **freepools** is not a NULL pointer, the first **pool** in the single linked list will be taken

![arena_orgnaize_allocate0](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate0.png)

**nfreepools** will be decremented

![arena_orgnaize_allocate2](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/memory_management/arena_orgnaize_allocate2.png)

#### arena overview part2

# read more
* [Memory management in Python](https://rushter.com/blog/python-memory-managment/)