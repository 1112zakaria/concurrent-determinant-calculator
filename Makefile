
all: DET

DET: DET.c DET.h
	gcc -o DET DET.c -g -W