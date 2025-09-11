build:
	gcc main.c http.c server.c websockets.c -o bin/main -g -lssl -lcrypto

run: build
	./bin/main

run.debug: build
	valgrind --tool=memcheck --leak-check=full ./bin/main

