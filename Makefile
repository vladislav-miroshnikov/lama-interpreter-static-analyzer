TARGET = lama-vm
CC=gcc
COMMON_FLAGS=-m32 -g2 -fstack-protector-all

all: gc_runtime.o runtime.o interpreter.o
	$(CC) $(COMMON_FLAGS) gc_runtime.o runtime.o main.o -o $(TARGET)

gc_runtime.o: runtime/gc_runtime.s
	$(CC) $(COMMON_FLAGS) -c runtime/gc_runtime.s

runtime.o: runtime/runtime.c runtime/runtime.h
	$(CC) $(COMMON_FLAGS) -c runtime/runtime.c

interpreter.o: main.c byte_file.h bytecode_decoder.h virtual_stack.h interpreter_state.h interpreter.h
	$(CC) $(COMMON_FLAGS) -c main.c

clean:
	$(RM) *.a *.o *~
