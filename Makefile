
all: main.c hp.c
	gcc -o hp main.c hp.c -pthread
