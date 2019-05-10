# re

### 目录

* [相关位置文件](#相关位置文件)
* [正则内部如何实现](#正则内部如何实现)
	* [parse](#parse)
	* [compile](#compile)
	* [match](#match)
* [代码细节](#代码细节)
	* [sre_ucs1_search](#sre_ucs1_search)
	* [fifo cache](#fifo-cache)
* [更多资料](#更多资料)

#### 相关位置文件
* cpython/Lib/re.py
* cpython/Lib/sre_compile.py
* cpython/Lib/sre_constants.py
* cpython/Lib/sre_parse.py
* cpython/Modules/sre.h
* cpython/Modules/sre_constants.h
* cpython/Modules/sre_lib.h
* cpython/Modules/_sre.c
* cpython/Modules/clinic/_sre.c.h

#### 正则内部如何实现

当你执行以下命令的时候发生了什么?

    import re
    re.search("abc\d+abc", "xxabc123abcd") # re.DEBUG)

调用栈

![call_stack](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/call_stack.png)

**re.search** 不同阶段的概览

![overview](https://github.com/zpoint/CPython-Internals/blob/master/Modules/re/overview.png)

我们看看在不同阶段都发生了什么

##### parse

 **parse** 阶段的核心代码在这个位置 **cpython/Lib/sre_parse.py**

有一个叫做 **Tokenizer** 的类负责把你输入的文本拆成一个一个的 token, **_parse** 函数负责把 token 和一些 **sre_constants** 绑定在一起

	# 伪代码
	tokenizer = Tokenizer(pattern, flags)
    while True:
    	next_token = tokenizer.get_next_token()
        if not next_token:
        	break
    	parse(next_token)

我们用前面输入的文本来跑一下 `abc\d+abc`

	next_token: a
	rest part of token: bc\d+abc
    parse result: [(LITERAL, 97)]

	next_token: b
	rest part of token: c\d+abc
    parse result: [(LITERAL, 97), (LITERAL, 98)]

    next_token: c
    rest part of token: \d+abc
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99)]

    next_token: \d
    rest part of token: +abc
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (IN, [(CATEGORY, CATEGORY_DIGIT)])]

现在 **next_token** 是 `'+'`, `parse` 函数会把当前最后一个结果取出, 加上这个 + 号代表的 symbol 之后再插回去

	item = parse_result[-1:]
    min, max = 1, MAXREPEAT
    parse_result[-1] = (MAX_REPEAT, (min, max, item))

    next_token: +
    rest part of token: abc
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])]))]

	next_token: a
	rest part of token: bc
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])), (LITERAL, 97)]

	next_token: b
	rest part of token: c
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])), (LITERAL, 97), (LITERAL, 98)]

    next_token: c
    rest part of token: None
    parse result: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])), (LITERAL, 97), (LITERAL, 98), (LITERAL, 99)]

##### compile

这是 **compile** 阶段的输入, 也是上一阶段的输出

    [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])), (LITERAL, 97), (LITERAL, 98), (LITERAL, 99)]

compile(编译) 这个阶段的函数在 **sre_compile.py** 这个文件里

    def _code(p, flags):

        flags = p.state.flags | flags
        code = []

        # compile info block
        _compile_info(code, p, flags)

        # compile the pattern
        _compile(code, p.data, flags)

        code.append(SUCCESS)

        return code

**info block** 具备以下的结构 `[INFO, 前缀code的长度, mask表示是否含有前缀, 当前的正则匹配成功所接受的最小长度, 接受的最大长度, 前缀1, 前缀2..., 前缀n, overlap_table_for_prefix]`

**_compile_info** 函数处理之后, code 变成

	code == [INFO, 12, 1, 7, MAXREPEAT, 3, 3, 97, 98, 99, 0, 0, 0]

12 表示当前的 code 前缀部分除开第一个 **INFO** 以外的长度为 12

1 是一个 mask, 值为 1 表示当前的 code 中包含文本前缀

7 是当前正则 `"abc\d+abc"` 所接受的最小长度的输入, 比如 `abc1abc` 长度为 7

MAXREPEAT s是当前正则 `"abc\d+abc"` 所接受的最大长度, 常量 **MAXREPEAT** 表示 +∞ (实际上最大长度也是有限制的)

第一个 3 是文本前缀的长度 (这里表示的是`"abc"`， 长为 3)

第二个 3 是 **prefix_skip** 的长度, **prefix_skip** 和文本前缀的长度相同

接下来的 97, 98, 99 表示文本前缀 `"abc"` (ascii 值)

接下来的 0, 0, 0 是函数 **_generate_overlap_table** 根据前缀生成的

编译完 **info block** 之后, 会继续编译 **pattern**, 这部分在 `_compile(code, p.data, flags)` 中

	for op, av in pattern:
    	compile_and_fill_code_according_to_op_and_av()

	# pattern 这里是: [(LITERAL, 97), (LITERAL, 98), (LITERAL, 99), (MAX_REPEAT, (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])), (LITERAL, 97), (LITERAL, 98), (LITERAL, 99)]

	op: LITERAL av: 97
	code: [LITERAL, 97]

	op: LITERAL av: 98
	code: [LITERAL, 97, LITERAL, 98]

	op: LITERAL av: 99
	code: [LITERAL, 97, LITERAL, 98, LITERAL, 99]

	op: MAX_REPEAT av: (1, MAXREPEAT, [(IN, [(CATEGORY, CATEGORY_DIGIT)])])

现在 op 为 MAX_REPEA, 对类型为 REPEAT 的处理方式有些许不同, 处理的代码段如下

    if op is MAX_REPEAT:
        emit(REPEAT_ONE)
    else:
        emit(MIN_REPEAT_ONE)
    skip = _len(code); emit(0)
    emit(av[0])
    emit(av[1])
    # 位置 1
    _compile(code, av[2], flags) # 递归调用
    emit(SUCCESS)
    code[skip] = _len(code) - skip # 把长度更改为真实长度
    # 位置 2

**code** 对象在 位置 1 如下

	code: [LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 0, 1, MAXREPEAT]

递归调用之后, **code** 对象在 位置 2 如下, length 现在变为 9

    op: IN av: [(CATEGORY, CATEGORY_DIGIT)]
    code: [LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 9, 1, MAXREPEAT, IN, 4, CATEGORY, CATEGORY_UNI_DIGIT, FAILURE, SUCCESS]

	op: LITERAL av: 97
	code: [LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 9, 1, MAXREPEAT, IN, 4, CATEGORY, CATEGORY_UNI_DIGIT, FAILURE, SUCCESS, LITERAL, 97]

	op: LITERAL av: 98
	code: [LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 9, 1, MAXREPEAT, IN, 4, CATEGORY, CATEGORY_UNI_DIGIT, FAILURE, SUCCESS, LITERAL, 97, LITERAL, 98]

	op: LITERAL av: 99
	code: [LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 9, 1, MAXREPEAT, IN, 4, CATEGORY, CATEGORY_UNI_DIGIT, FAILURE, SUCCESS, LITERAL, 97, LITERAL, 98, LITERAL, 99, SUCCESS]

**_compile_info** 和 **_compile** 都处理完后, code 对象如下所示, 会作为下一阶段的输入

    [INFO, 12, 1, 7, MAXREPEAT, 3, 3, 97, 98, 99, 0, 0, 0, LITERAL, 97, LITERAL, 98, LITERAL, 99, REPEAT_ONE, 9, 1, MAXREPEAT, IN, 4, CATEGORY, CATEGORY_UNI_DIGIT, FAILURE, SUCCESS, LITERAL, 97, LITERAL, 98, LITERAL, 99, SUCCESS]

##### match

match 阶段是用 c 完成的

核心部分是一个超大的 for 循环, 以这样一种方式来完成自动机的转换, 每一个上一阶段编译后的 opcode 会被传到这里, 根据不同的 opcode 进入不同的分支执行不同的代码段, 比如如果是文本 ascii 值 97, 就单纯的比较 97 和字符串对应的值是否相同, 如果是 +, 就按照 + 的方式处理, 成功跳转到下一步, 失败跳转到失败, 可以参考下自动机理论

实现方式和 python 虚拟机中的 ceval.c 类似, 如果时间允许, 会专门写一篇 match 这一阶段的实现

    for (;;) {
    	switch (pattern[i]) {
            case SRE_OP_MARK:xxx
            case SRE_OP_LITERAL:xxx
            case SRE_OP_REPEAT_ONE:xxx
        	...
        }
    }

#### code 代码细节

##### sre_ucs1_search

根据调用栈找到这的时候

	status = sre_search(&state, PatternObject_GetCode(self));

**sre_search** 的定义如下

搜索会根据字符串的不同字节占用数而指派到不同的函数上面去干活, **unicode** 的不同类型可以参加之前的 [unicode 对象的实现](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/str/str_cn.md)

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

我们在 cpython 文件夹下搜索 **sre_ucs1_search** 这个函数是搜不到内容的

	find . -name '*' -exec grep -nHr "sre_ucs1_search" {} \;
    Binary file ./libpython3.8m.a matches
	./Modules/_sre.c:572:        return sre_ucs1_search(state, pattern);

这里的原因是要似的 **sre_lib.h** 这个文件的公共部分导入多次, 上面的函数只能用##拼接的形式定义

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

我们现在能在 **sre_lib.h** 中找到 **search** 了

    LOCAL(Py_ssize_t)
    SRE(search)(SRE_STATE* state, SRE_CODE* pattern)

展开之后会有三种不同的形式

	inline Py_ssize_t sre_ucs1_search(SRE_STATE* state, SRE_CODE* pattern)
    inline Py_ssize_t sre_ucs2_search(SRE_STATE* state, SRE_CODE* pattern)
    inline Py_ssize_t sre_ucs4_search(SRE_STATE* state, SRE_CODE* pattern)

进入展开后的函数, 就能找到 [match](#match) 部分的代码了

#### fifo cache

当你调用 **re.compile** 时, python 层级实现了一个 fifo 缓存

**_cache** 的类型是字典, 因为字典对象现在是有序的了([字典对象实现](https://github.com/zpoint/CPython-Internals/blob/master/BasicObject/dict/dict.md#delete)), 就可以把他当成一个 fifo 队列使用

    >>> r1 = re.compile("a\d+b")
    >>> id(r1)
    4409698992
    >>> r2 = re.compile("b\d+b")
    >>> id(r2)
    4409699184
    >>> r3 = re.compile("a\d+b")
    >>> id(r3)
    4409698992 # 和 r1 相同
    >>> r4 = re.compile("b\d+b")
    >>> id(r4)
    4409699184 # 和 r2 相同

cache 机制的 python 实现代码

    _cache = {}  # ordered!

    _MAXCACHE = 512
    def _compile(pattern, flags):
    	# 忽略部分代码
        try:
            return _cache[type(pattern), pattern, flags]
        except KeyError:
            pass
		# 忽略部分代码
        p = sre_compile.compile(pattern, flags)
        if not (flags & DEBUG):
            if len(_cache) >= _MAXCACHE:
                # Drop the oldest item
                try:
                    del _cache[next(iter(_cache))]
                except (StopIteration, RuntimeError, KeyError):
                    pass
            _cache[type(pattern), pattern, flags] = p
        return p

#### 更多资料
* [ccs.neu.edu->sre](http://www.ccs.neu.edu/home/shivers/papers/sre.txt)
* [Python's Hidden Regular Expression Gems](http://lucumr.pocoo.org/2015/11/18/pythons-hidden-re-gems/)
* [Understanding Python’s SRE structure](https://blog.labix.org/2003/06/16/understanding-pythons-sre-structure)
* [Get Started with Regex: Regular Expressions Make Easy](https://www.whoishostingthis.com/resources/regex/)
* [Comparing regular expressions in Perl, Python, and Emacs](https://www.johndcook.com/blog/regex-perl-python-emacs/)
