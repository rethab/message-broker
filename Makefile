CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall
TEST_CFLAGS=-Itest -lcunit -lpthread -g -rdynamic

all: broker

broker: src/broker.c topic stomp
	gcc $(CFLAGS) -o src/broker src/broker.c src/stomp.o src/topic.o

client: src/client.c stomp
	gcc $(CFLAGS) -o src/client src/client.c src/stomp.o 

test: topic stomp socket
	gcc $(CFLAGS) $(TEST_CFLAGS) -o test/main.o test/main.c src/topic.o src/stomp.o src/socket.o


topic: src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

socket: src/socket.c
	gcc -c $(CFLAGS) -o src/socket.o src/socket.c

clean:
	rm -fv test/*.o
	rm -fv src/*.o
	rm -fv src/broker
	rm -fv src/client
