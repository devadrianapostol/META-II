CC=gcc
CFLAGS=-c -g -Wall -Wconversion -Wno-switch -Wno-parentheses -Wno-sign-conversion

all: meta_machine meta_machine_bt meta_compiler valgol_machine META_II.m2a VALGOL_I.m2a

meta_machine: META_II_machine.o asm.o
	$(CC) -o meta_machine META_II_machine.o asm.o

meta_machine_bt: META_II_machine_bt.o asm.o
	$(CC) -o meta_machine_bt META_II_machine_bt.o asm.o

valgol_machine: VALGOL_I_machine.o asm.o
	$(CC) -o valgol_machine VALGOL_I_machine.o asm.o

meta_compiler: META_II_compiler.o
	$(CC) -o meta_compiler META_II_compiler.o

META_II_machine.o: META_II_machine.c asm.h
	$(CC) $(CFLAGS) META_II_machine.c

META_II_machine_bt.o: META_II_machine_bt.c asm.h
	$(CC) $(CFLAGS) META_II_machine_bt.c

VALGOL_I_machine.o: VALGOL_I_machine.c asm.h
	$(CC) $(CFLAGS) VALGOL_I_machine.c

META_II_compiler.o: META_II_compiler.c
	$(CC) $(CFLAGS) META_II_compiler.c

asm.o: asm.c asm.h
	$(CC) $(CFLAGS) asm.c

META_II.m2a: meta_compiler meta_machine
	./meta_compiler META_II.m2 > META_II.m2a
	./meta_machine META_II.m2a META_II.m2 > _META_II.m2a
	cmp META_II.m2a _META_II.m2a

VALGOL_I.m2a: meta_machine META_II.m2a valgol_machine
	./meta_machine META_II.m2a VALGOL_I.m2 > VALGOL_I.m2a
	./meta_machine VALGOL_I.m2a VALGOL_I_example >ex.v1a
	./valgol_machine ex.v1a >VALGOL_I_example.output
	cmp VALGOL_I_example.output VALGOL_I_example.expect
	rm -f ex.v1a VALGOL_I_example.output

clean:
	rm -f *.o meta_machine meta_machine_bt meta_compiler valgol_machine META_II.m2a _META_II.m2a VALGOL_I.m2a

.PHONY: all clean

