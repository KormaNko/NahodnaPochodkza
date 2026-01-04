CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -O2 -g
INCLUDES=-Iinclude
LDLIBS=-pthread

COMMON_SRC=src/common/net.c src/common/config.c
SERVER_SRC=src/server/server.c
CLIENT_SRC=src/client/client.c

.PHONY: all server client clean

all: server client

server:
	@mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDES) $(SERVER_SRC) $(COMMON_SRC) -o bin/server $(LDLIBS) -lm

client:
	@mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDES) $(CLIENT_SRC) $(COMMON_SRC) -o bin/client $(LDLIBS)

clean:
	rm -rf bin