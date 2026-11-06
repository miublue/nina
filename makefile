CC = tcc
CFLAGS = -Wall -Werror -Wextra
SRCFILES = analyzer.c codegen.c lexer.c main.c parser.c

all:
	${CC} -o ninac ${SRCFILES} ${CFLAGS}

test: all
	./ninac examples/test.ni -k -o test
	./test

clean:
	rm ninac test __tmp_output.c

.PHONY: all test clean

