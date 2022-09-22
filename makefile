CC = cl65
CODE_FILES =  main.c instructions.c functions.s dyn_list.c
HEADERS = main.h instructions.h functions.h dyn_list.h

all: xasm.prg
	
xasm.prg: $(CODE_FILES) $(HEADERS)
	$(CC) $(CODE_FILES) -Cl --cpu 65c02 -t cx16 -o XASM.PRG