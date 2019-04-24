# fileio

### category

* [related file](#related-file)
* [memory layout](#memory-layout)
* [method](#method)
	* [new](#new)
	* [add](#add)
		* [hash collision](#hash-collision)
		* [resize](#resize)
	    * [why LINEAR_PROBES?](#why-LINEAR_PROBES)
	* [clear](#clear)

#### related file
* cpython/Modules/_io/fileio.c

#### memory layout

![memory layout](https://github.com/zpoint/Cpython-Internals/blob/master/Modules/io/fileio/layout.png)

