build:
	gcc main.c http.c server.c -o bin/main

run: build
	./bin/main
