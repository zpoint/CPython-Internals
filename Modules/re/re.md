# re

### contents

* [related file](#related-file)
* [how regex work](#how-regex-work)
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

what happened when you executing the following script ?

    import re
    re.search("abc\dabc", "xxabc123abcd") # re.DEBUG)

this is the call stack

![call_stack](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/call_stack.png)

let's see what's going on in each step



#### reference
* [ccs.neu.edu->sre](http://www.ccs.neu.edu/home/shivers/papers/sre.txt)
* [Python's Hidden Regular Expression Gems](http://lucumr.pocoo.org/2015/11/18/pythons-hidden-re-gems/)
* [Understanding Python’s SRE structure](https://blog.labix.org/2003/06/16/understanding-pythons-sre-structure)
* [Get Started with Regex: Regular Expressions Make Easy](https://www.whoishostingthis.com/resources/regex/)
* [Comparing regular expressions in Perl, Python, and Emacs](https://www.johndcook.com/blog/regex-perl-python-emacs/)
