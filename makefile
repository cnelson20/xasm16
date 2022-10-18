CC = gcc
CODE_FILES =  main.c instructions.c functions.c dyn_list.c
HEADERS = main.h instructions.h functions.h dyn_list.h

all: xasm.prg
	
xasm.prg: $(CODE_FILES) $(HEADERS)
	$(CC) $(CODE_FILES) -o xasm
