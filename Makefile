CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall
TEST_CFLAGS=-Itest -lcunit -g -rdynamic

all: broker

broker: broker.c
	gcc $(CFLAGS) -o broker broker.c

test: test/main.o
	test/main.o

test/main.o: topic stomp
	gcc $(CFLAGS) $(TEST_CFLAGS) -o test/main.o test/main.c src/topic.o src/stomp.o

topic: src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

clean:
	rm -v test/*.o
	rm -v src/*.o
	rm -v broker
