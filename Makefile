CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall -lpthread 
TEST_CFLAGS=-Itest -lcunit -g -rdynamic

all: broker

broker: src/broker.c topic stomp
	gcc $(CFLAGS) -o src/broker src/broker.c src/stomp.o src/topic.o

client: src/client.c stomp
	gcc $(CFLAGS) -o src/client src/client.c src/stomp.o 

test: CFLAGS += $(TEST_CFLAGS)
test: clean topic stomp socket worker
	gcc $(CFLAGS) -o test/main.o test/main.c src/topic.o src/stomp.o src/socket.o src/worker.o

topic: src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

socket: src/socket.c
	gcc -c $(CFLAGS) -o src/socket.o src/socket.c

worker: src/worker.c
	gcc -c $(CFLAGS) -o src/worker.o src/worker.c

clean:
	rm -fv test/*.o
	rm -fv src/*.o
	rm -fv src/broker
	rm -fv src/client
