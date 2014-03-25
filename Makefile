test: stomp-test.o
	test/stomp-test.o

stomp-test.o:
	gcc -Isrc -Itest -lcunit -o test/stomp-test.o test/stomp-test.c src/stomp.c
