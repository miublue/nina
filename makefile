CC = tcc
CFLAGS = -Wall -Werror -Wextra
SRCFILES = analyzer.c codegen.c lexer.c main.c module.c parser.c
OBJFILES = ${SRCFILES:.c=.o}
PREFIX = /usr/local

all: ninac

ninac: ${OBJFILES}
	${CC} -o $@ $^ ${CFLAGS}

test: all
	./ninac examples/test.ni -k -o test
	./test

clean:
	rm -f ninac test __tmp_output.c *.o

install: all
	mkdir -p ${PREFIX}/bin ${PREFIX}/lib/nina
	install -s ninac ${PREFIX}/bin/
	cp -r std ${PREFIX}/lib/nina/

uninstall:
	rm -rf ${PREFIX}/bin/ninac ${PREFIX}/lib/nina

.PHONY: all test clean install uninstall
