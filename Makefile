all: main

main: main.o
	gcc -o chip8 main.o -lSDL2

main.o: main.c
	gcc -c main.c -lSDL2

clean:
	rm -f *.o chip8
