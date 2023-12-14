COMMON_FLAGS=-m32 -g2 -fstack-protector-all -O -lm


all: util.o set.o

util.o: util.c util.h
	gcc $(COMMON_FLAGS) -c util.c -o util.o

set.o: set.c set.h
	gcc $(COMMON_FLAGS) -c set.c -o set.o

clean:
	$(RM) *.a *.o *~

