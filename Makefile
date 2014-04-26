CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall -lpthread 
TEST_CFLAGS=-Itst -lcunit -g -rdynamic -ftest-coverage -fprofile-arcs -lgcov
PROD_CFLAGS=-DNDEBUG -Wno-unused-but-set-variable -Wno-unused-variable

all: server client
	cp -vf src/server run
	cp -vf tst/client test

server: CFLAGS += $(PROD_CFLAGS)
server: topic stomp broker socket distributor gc
	gcc $(CFLAGS) -o src/server src/server.c src/stomp.o src/topic.o src/broker.o src/socket.o src/distributor.o src/gc.o src/list.o

client: CFLAGS += $(PROD_CFLAGS)
client: stomp
	gcc $(CFLAGS) -o tst/client tst/client.c src/stomp.o 

test: CFLAGS += $(TEST_CFLAGS)
test: clean topic stomp socket broker distributor gc
	gcc $(CFLAGS) -o tst/main.o tst/main.c src/topic.o src/stomp.o src/socket.o src/broker.o src/distributor.o src/gc.o src/list.o

cover: test
	tst/main.o
	lcov -c -d src -o coverage.info
	lcov --remove coverage.info "/usr*" -o coverage.info
	genhtml coverage.info -o coverage

topic: list src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

socket: src/socket.c
	gcc -c $(CFLAGS) -o src/socket.o src/socket.c

broker: src/broker.c
	gcc -c $(CFLAGS) -o src/broker.o src/broker.c

distributor: src/distributor.c
	gcc -c $(CFLAGS) -o src/distributor.o src/distributor.c

gc: src/gc.c
	gcc -c $(CFLAGS) -o src/gc.o src/gc.c

list: src/list.c
	gcc -c $(CFLAGS) -o src/list.o src/list.c

clean:
	rm -fv {src,tst}/*.o
	rm -fv src/server
	rm -fv tst/client
	rm -rfv coverage/
	rm -fv coverage.info
	rm -fv {src/,}*.gcda
	rm -fv {src/,}*.gcno
	rm -fv run
	rm -fv test
