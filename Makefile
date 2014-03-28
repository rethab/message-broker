CFLAGS=-Isrc -std=c99 -D_XOPEN_SOURCE=700
TEST_CFLAGS=-Itest -lcunit -g -rdynamic

test: stomp-test.o
	test/stomp-test.o

stomp-test.o:
	gcc $(CFLAGS) $(TEST_CFLAGS) -o test/stomp-test.o test/stomp-test.c src/stomp.c


clean:
	rm test/stomp-test.o
