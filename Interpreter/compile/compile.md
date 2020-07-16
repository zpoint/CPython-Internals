# compile

# contents

* [related file](#related-file)
* [tokenizer](#tokenizer)

# related file

* Python/pythonrun.c
* Parser/tokenizer.c
* Parser/tokenizer.h
* Parser/parsetok.c

# tokenizer

Let's begin with the python Interactive loop, and execute a simple line

```pythoon3
./python.exe
>>> a1 = 10 + 2
```

Token parser will scan the input and prase your input string according to the function written in `Parser/tokenizer.c->tok_get`



```bash
make regen-grammar
```

