CC=gcc
CFLAGS=-Wall -ggdb -O0
LDFLAGS=-pthread -lm

EXTRA_CFLAGS=`pkg-config --cflags gl glut glew`
EXTRA_LDFLAGS=`pkg-config --libs gl glut glew`

all: query2-info

query2-info: query2-info.c util.h util.c util-string.h util-string.c
	$(CC) query2-info.c util.c util-string.c -o query2-info $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS)

clean:
	rm -f query2-info

