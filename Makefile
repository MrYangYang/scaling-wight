CC = gcc
LIBS = 
CFLAGS = 

myls: myls.o
	gcc -g -Wall myls.o -o myls

clean:
	rm myls.o myls

