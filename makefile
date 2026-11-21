CC = tcc
CFLAGS = -Wall -Werror -Wextra -Isrc
SRCFILES = ${wildcard src/*.c}
OBJFILES = ${SRCFILES:.c=.o}
PREFIX = /usr/local

all: ninac

ninac: ${OBJFILES}
	${CC} -o $@ $^ ${CFLAGS}

test: all
	./ninac examples/hello.ni && ./a.out
	./ninac examples/arrays.ni && ./a.out
	./ninac examples/loops.ni && ./a.out

clean:
	rm -f ninac __tmp_output.c *.o *.out

install: all
	mkdir -p ${PREFIX}/bin ${PREFIX}/lib/nina
	install -s ninac ${PREFIX}/bin/
	cp -r std ${PREFIX}/lib/nina/

uninstall:
	rm -rf ${PREFIX}/bin/ninac ${PREFIX}/lib/nina

.PHONY: all test clean install uninstall
