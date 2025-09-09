build:
	gcc main.c http.c -o bin/main

run: build
	./bin/main
