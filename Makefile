# very simple - just a test/example case
CFLAGS  += -c -std=c89 -Wall -Wextra -Werror -O2 `pkg-config --cflags cairo`
LDFLAGS += -L/usr/X11R6/lib `pkg-config --libs cairo` -lxcb -lxcb-icccm

.PHONY: clean

all: example

.c.o:
	$(CC) $(CFLAGS) $<

example: example.o
	$(CC) -o $@ $(LDFLAGS) example.o

clean:
	rm -f example example.o example.core
