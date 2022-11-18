
CFLAGS=-g -DTESTING_VERBOSE -DTHREAD_VERBOSE -Wno-deprecated-declarations
LIBS=-pthread  -lm

all: lab2main

lab2main: lab2main.o common.o
	$(CC) -o $@ $^ $(LIBS)
	
common.o: common.c common.h
lab2main.o: lab2main.c common.h

clean: 
	rm *.o lab2main
