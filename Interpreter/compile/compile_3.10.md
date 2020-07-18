# compile

# contents

* [related file](#related-file)
* [tokenizer](#tokenizer)

The following contents are based on version 3.10.0 alpha 0

```bash
git reset --hard 38d3864efe914fda64553e2ec75c9ec15574483f
```

# related file

* Python/pythonrun.c
* Parser/tokenizer.c
* Parser/tokenizer.h
* Parser/parsetok.c

```bash
% make regen-pegen
PYTHONPATH=./Tools/peg_generator python3 -m pegen -q c \
                ./Grammar/python.gram \
                ./Grammar/Tokens \
                -o ./Parser/parser.new.c
python3 ./Tools/scripts/update_file.py ./Parser/parser.c ./Parser/parser.new.c
% make regen-keyword
\# Regenerate Lib/keyword.py from Grammar/python.gram and Grammar/Tokens
# using Tools/peg_generator/pegen
PYTHONPATH=./Tools/peg_generator python3 -m pegen.keywordgen \
                ./Grammar/python.gram \
                ./Grammar/Tokens \
                ./Lib/keyword.py.new
python3 ./Tools/scripts/update_file.py ./Lib/keyword.py ./Lib/keyword.py.new
```



# read more

* [parsing expression grammar](https://en.wikipedia.org/wiki/Parsing_expression_grammar)

