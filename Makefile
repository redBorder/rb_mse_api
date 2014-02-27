CFLAGS=-W -Wall -O3 -g -fpic
#CFLAGS=-W -Wall -O0 -g -fpic -D_FORTIFY_SOURCE=2 

DESTDIR=/opt/rb

# If you have librd in another location than the default
CFLAGS+=-I/opt/rb/include 
LDFLAGS+=-L/opt/rb/lib

all: rb_mse_api.o librb_mse_api.so

rb_mse_api.o: rb_mse_api.c rb_mse_api.h
	cc ${CFLAGS} -o $@ $< -c

strbuffer.o: strbuffer.c strbuffer.h
	cc ${CFLAGS} -o $@ $< -c

librb_mse_api.so: rb_mse_api.o strbuffer.o
	cc -shared -o $@ $^  $(LDFLAGS) -lcurl -ljansson -lrd

examples: examples.c rb_mse_api.o strbuffer.o 
	cc ${CFLAGS} ${LDFLAGS} -o $@ $^ -lcurl -ljansson -lrd

install: rb_mse_api.h librb_mse_api.so
	install -t $(DESTDIR)/include rb_mse_api.h
	install -t $(DESTDIR)/lib     librb_mse_api.so

clean:
	rm -rf *.o examples
