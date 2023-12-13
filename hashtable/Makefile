COMMON_FLAGS=-m32 -g2 -fstack-protector-all -O -lm


all: hashtable.o hashtable_itr.o hashtable_utility.o

hashtable.o: hashtable.c
	gcc $(COMMON_FLAGS) -c hashtable.c -o hashtable.o

hashtable_itr.o: hashtable_itr.c
	gcc $(COMMON_FLAGS) -c hashtable_itr.c -o hashtable_itr.o

hashtable_utility.o: hashtable_utility.c
	gcc $(COMMON_FLAGS) -c hashtable_utility.c -o hashtable_utility.o

clean:
	$(RM) *.a *.o *~

