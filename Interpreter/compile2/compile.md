# CST TO AST

# contents

* [related file](#related-file)
* [read more](#read-more)

# related file

* Python/ast.c

* Python/pythonrun.c

  

The following command will generate `Python-ast.h` and `Python-ast.c` from `Parser/Python.asdl`, which will be used to construct `AST` from the previos [parse tree](https://github.com/zpoint/CPython-Internals/blob/master/Interpreter/compile/compile.md#parse)

```shell
% make regen-ast
# Regenerate Include/Python-ast.h using Parser/asdl_c.py -h
./install-sh -c -d ./Include
python3 ./Parser/asdl_c.py \
                -h ./Include/Python-ast.h.new \
                ./Parser/Python.asdl
python3 ./Tools/scripts/update_file.py ./Include/Python-ast.h ./Include/Python-ast.h.new
# Regenerate Python/Python-ast.c using Parser/asdl_c.py -c
./install-sh -c -d ./Python
python3 ./Parser/asdl_c.py \
                -c ./Python/Python-ast.c.new \
                ./Parser/Python.asdl
python3 ./Tools/scripts/update_file.py ./Python/Python-ast.c ./Python/Python-ast.c.new

```

> * Lowercase names are non-terminals.
> * Uppercase names are terminals.
> * Literal tokens are in double quotes.
> * `[]` means zero or one.
> * `{}` means one or more.
> * `?` means it is optional, `*` means 0 or more



# read more

* [using-asdl-to-describe-asts-in-compilers](https://eli.thegreenplace.net/2014/06/04/using-asdl-to-describe-asts-in-compilers)
* [What is Zephyr ASDL](https://www.oilshell.org/blog/2016/12/11.html)
* [Python's compiler - from CST to AST](https://aoik.me/blog/posts/python-compiler-from-cst-to-ast)
* [Design of CPython’s Compiler](https://devguide.python.org/compiler/)

