CC = gcc
CODE_FILES = main.o instructions.o functions.o dyn_list.o
HEADERS = main.h instructions.h functions.h dyn_list.h

all: $(CODE_FILES)
	$(CC) $(CODE_FILES) -g3 -o xasm

main.o: main.c main.h
	gcc -c main.c

instructions.o: instructions.c instructions.h
	gcc -c instructions.c

functions.o: functions.c functions.h
	gcc -c functions.c

dyn_list.o: dyn_list.c dyn_list.h
	gcc -c dyn_list.c
