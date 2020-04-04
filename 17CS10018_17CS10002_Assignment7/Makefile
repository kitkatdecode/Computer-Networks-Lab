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
			# gcc user1.c -o s -L ./ -lrsocket
			# gcc user2.c -o r -L ./ -lrsocket
			@echo "static library librsocket created"
			@echo "compile your program with commands \ngcc user1.c -o s -L ./ -lrsocket && gcc user2.c -o r -L ./ -lrsocket"
			
$(OBJS):	rsocket.c
			$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -f *.o
	rm -f *.a
