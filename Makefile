CFLAGS = -I./include/SDL2
LDIR = -L./lib
LIBS = -lmingw32 -lSDL2main -lSDL2

Snakee : main.c
	gcc $(LDIR) -mwindows -o Snakee main.c $(CFLAGS) $(LIBS)