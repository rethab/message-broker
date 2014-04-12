CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall
TEST_CFLAGS=-Itest -lcunit -g -rdynamic

all: broker

broker: src/broker.c topic stomp
	gcc $(CFLAGS) -o src/broker src/broker.c src/stomp.o src/topic.o

test: test/main.o
	test/main.o

test/main.o: topic stomp
	gcc $(CFLAGS) $(TEST_CFLAGS) -o test/main.o test/main.c src/topic.o src/stomp.o

topic: src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

clean:
	rm -fv test/*.o
	rm -fv src/*.o
