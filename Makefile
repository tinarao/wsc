build:
	gcc main.c http.c server.c websockets.c -o bin/main

run: build
	./bin/main
