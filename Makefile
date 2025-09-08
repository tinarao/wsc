build:
	gcc main.c -o bin/main

run: build
	./bin/main
