CC = gcc
CFLAGS =-Wall -std=c99 -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -g
PROG = fs


SRCS = *.c



all: $(PROG) 

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) $(SRCS) `pkg-config fuse --cflags --libs` -lm -o $(PROG)


clean:
	rm -f $(PROG) $(MKFS)
