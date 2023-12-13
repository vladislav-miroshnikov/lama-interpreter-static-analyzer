TARGET = lama-vm
CC=gcc
COMMON_FLAGS=-m32 -g2 -fstack-protector-all

all: gc_runtime.o runtime.o build_hashtable vm.o
	$(CC) $(COMMON_FLAGS) gc_runtime.o runtime.o hashtable/hashtable.o hashtable/hashtable_itr.o hashtable/hashtable_utility.o main.o -o $(TARGET)

gc_runtime.o: runtime/gc_runtime.s
	$(CC) $(COMMON_FLAGS) -c runtime/gc_runtime.s

runtime.o: runtime/runtime.c runtime/runtime.h
	$(CC) $(COMMON_FLAGS) -c runtime/runtime.c

vm.o: main.c byte_file.h bytecode_decoder.h interpreter.h analyzer/analyzer.h
	$(CC) $(COMMON_FLAGS) -c main.c

build_hashtable:
	make -C hashtable all

clean:
	make -C hashtable clean
	$(RM) *.a *.o *~
