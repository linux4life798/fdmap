CFLAGS += -g -O0
CFLAGS += -Wall

all: libfdmap.a

libfdmap.a: libfdmap.a(fdmap_list.o fdmap.o)

clean:
	$(RM) *.o *.a
