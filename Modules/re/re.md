# re

### contents

* [related file](#related-file)
* [how regex work](#how-regex-work)
	* [parse](#parse)
	* [compile](#compile)
	* [match](#match)
* [code detail](#code-detail)
* [reference](#reference)

#### related file
* cpython/Lib/re.py
* cpython/Lib/sre_compile.py
* cpython/Lib/sre_constants.py
* cpython/Lib/sre_parse.py
* cpython/Modules/sre.h
* cpython/Modules/sre_constants.h
* cpython/Modules/sre_lib.h
* cpython/Modules/_sre.c
* cpython/Modules/clinic/_sre.c.h

#### how regex work

what happened when you execute the following code ?

    import re
    re.search("abc\dabc", "xxabc123abcd") # re.DEBUG)

the call stack

![call_stack](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/call_stack.png)

the overview of different phases of **re.search**

![overview](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/overview.png)

let's see what's going on in each step

##### parse



##### compile

##### match

#### code detail

follow the call stack to here

	status = sre_search(&state, PatternObject_GetCode(self));

the defination of **sre_search** is

    LOCAL(Py_ssize_t)
    sre_search(SRE_STATE* state, SRE_CODE* pattern)
    {
        if (state->charsize == 1)
            return sre_ucs1_search(state, pattern);
        if (state->charsize == 2)
            return sre_ucs2_search(state, pattern);
        assert(state->charsize == 4);
        return sre_ucs4_search(state, pattern);
    }

if we search for **sre_ucs1_search** in the folder, we can't find anything

	find . -name '*' -exec grep -nHr "sre_ucs1_search" {} \;
    Binary file ./libpython3.8m.a matches
	./Modules/_sre.c:572:        return sre_ucs1_search(state, pattern);

the trick here is for reusing the common part in **sre_lib.h** with different defination

    /* generate 8-bit version */

    #define SRE_CHAR Py_UCS1
    #define SIZEOF_SRE_CHAR 1
    #define SRE(F) sre_ucs1_##F
    #include "sre_lib.h"

    /* generate 16-bit unicode version */

    #define SRE_CHAR Py_UCS2
    #define SIZEOF_SRE_CHAR 2
    #define SRE(F) sre_ucs2_##F
    #include "sre_lib.h"

    /* generate 32-bit unicode version */

    #define SRE_CHAR Py_UCS4
    #define SIZEOF_SRE_CHAR 4
    #define SRE(F) sre_ucs4_##F
    #include "sre_lib.h"

we can find the defination of **search** in **sre_lib.h**

    LOCAL(Py_ssize_t)
    SRE(search)(SRE_STATE* state, SRE_CODE* pattern)

which will be expanded to three different form

	inline Py_ssize_t sre_ucs1_search(SRE_STATE* state, SRE_CODE* pattern)
    inline Py_ssize_t sre_ucs2_search(SRE_STATE* state, SRE_CODE* pattern)
    inline Py_ssize_t sre_ucs4_search(SRE_STATE* state, SRE_CODE* pattern)

when I go inside the expanded **sre_ucs1_search**, I found the [match](#match) phase



#### reference
* [ccs.neu.edu->sre](http://www.ccs.neu.edu/home/shivers/papers/sre.txt)
* [Python's Hidden Regular Expression Gems](http://lucumr.pocoo.org/2015/11/18/pythons-hidden-re-gems/)
* [Understanding Python’s SRE structure](https://blog.labix.org/2003/06/16/understanding-pythons-sre-structure)
* [Get Started with Regex: Regular Expressions Make Easy](https://www.whoishostingthis.com/resources/regex/)
* [Comparing regular expressions in Perl, Python, and Emacs](https://www.johndcook.com/blog/regex-perl-python-emacs/)
