PROG =		librsocket.a

OBJS =		rsocket.o

CC =		gcc

CFLAGS+= 	-Wall -I. -I../common
CFLAGS+= 	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= 	-Wmissing-declarations
CFLAGS+= 	-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+= 	-Wsign-compare

$(PROG):	$(OBJS)
			ar rcs $@ $<

$(OBJS):	rsocket.c
			$(CC) -o $@ $(CFLAGS) $<