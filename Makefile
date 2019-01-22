CFLAGS=-Wall -std=c11 
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

ccc: $(OBJS)
	cc -o $@ $(OBJS) 

$(OBJS): ccc.h

test: ccc
	./ccc -test
	./test.sh

clean:
	rm -f ccc *.o *~ tmp tmp.s