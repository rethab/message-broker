CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700 -Wall -lpthread 
TEST_CFLAGS=-Itest -lcunit -g -rdynamic -ftest-coverage -fprofile-arcs -lgcov
PROD_CFLAGS=-DNDEBUG

server: CFLAGS += $(PROD_CFLAGS)
server: topic stomp broker socket distributor
	gcc $(CFLAGS) -o src/server src/server.c src/stomp.o src/topic.o src/broker.o src/socket.o src/distributor.o

client: CFLAGS += $(PROD_CFLAGS)
client: stomp
	gcc $(CFLAGS) -o src/client src/client.c src/stomp.o 

test: CFLAGS += $(TEST_CFLAGS)
test: clean topic stomp socket broker distributor
	gcc $(CFLAGS) -o test/main.o test/main.c src/topic.o src/stomp.o src/socket.o src/broker.o src/distributor.o

cover: test
	test/main.o
	lcov -c -d src -o coverage.info
	lcov --remove coverage.info "/usr*" -o coverage.info
	genhtml coverage.info -o coverage

topic: src/topic.c
	gcc -c $(CFLAGS) -o src/topic.o src/topic.c

stomp: src/stomp.c
	gcc -c $(CFLAGS) -o src/stomp.o src/stomp.c

socket: src/socket.c
	gcc -c $(CFLAGS) -o src/socket.o src/socket.c

broker: src/broker.c
	gcc -c $(CFLAGS) -o src/broker.o src/broker.c

distributor: src/distributor.c
	gcc -c $(CFLAGS) -o src/distributor.o src/distributor.c

clean:
	rm -fv {src,test}/*.o
	rm -fv src/{server,client}
	rm -rfv coverage/
	rm -fv coverage.info
	rm -fv {src/,}*.gcda
	rm -fv {src/,}*.gcno
