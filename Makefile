CC = gcc
LIBS = `pkg-config --libs glib-2.0`
CFLAGS = `pkg-config --cflags glib-2.0`

myls: myls.o
	gcc -g -Wall myls.o -o myls

clean:
	rm myls.o myls

