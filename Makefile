CFLAGS=-Wall -std=c11 
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

cncc: $(OBJS)
	cc -o $@ $(OBJS) 

$(OBJS): cncc.h

legacy_test: cncc
	./cncc -test
	./test.sh

test: cncc tests/test.c
	./cncc -test
	
	@gcc -E -P tests/test.c > tmp-test.tmp
	@./cncc -file tmp-test.tmp > tmp.s
	@echo 'int global_arr[1]={5};' | gcc -xc -c -o tmp-test.o -
	@gcc -static -g -o tmp-test tmp.s tmp-test.o
	@./tmp-test

clean:
	rm -f cncc *.o *~ tmp* test/*~ a.out *log*.txt