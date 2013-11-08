#CFLAGS=-O3 -g
CFLAGS=-W -Wall -O0 -g

all: rb_mse_api.o

rb_mse_api.o: rb_mse_api.c rb_mse_api.h
	cc ${CFLAGS} -o $@ $< -c

examples: examples.c rb_mse_api.o
	cc ${CFLAGS} -o $@ $^ -lcurl

clean:
	rm -rf *.o examples
