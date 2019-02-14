CFLAGS=-Wall -std=c11 
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

ccc: $(OBJS)
	cc -o $@ $(OBJS) 

$(OBJS): ccc.h

legacy_test: ccc
	./ccc -test
	./test.sh

test: ccc tests/test.c
	./ccc -test
	
	@./ccc "$$(gcc -E -P tests/test.c)" > tmp.s
	@echo 'int global_arr[1]={5};' | gcc -xc -c -o tmp-test.o -
	@gcc -static -o tmp-test tmp.s tmp-test.o
	@./tmp-test

clean:
	rm -f ccc *.o *~ tmp* test/*~ a.out