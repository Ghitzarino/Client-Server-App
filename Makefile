# Makefile

CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

common.o: common.c

# Compileaza server.c
server: server.c common.o
	$(CC) $(CFLAGS) -o server server.c common.o

# Compileaza subscriber.c
subscriber: subscriber.c common.o
	$(CC) $(CFLAGS) -o subscriber subscriber.c common.o

.PHONY: clean run_server run_subscriber

clean:
	rm -rf server subscriber *.o *.dSYM
