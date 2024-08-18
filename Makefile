game: main.c
	mkdir -p bin
	gcc -O3 -Wall -o bin/game main.c -Iinclude/ -Llib lib/libraylib.a -lraylib -lm -ldl