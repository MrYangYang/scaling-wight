CC = gcc
LIBS = -lglib-2.0
CFLAGS = -I/usr/include/glib-2.0 \
		 -I/usr/lib/glib-2.0/include

myls: myls.o
	gcc -g -Wall $(CFLAGS) $(LIBS) myls.o -o myls

clean:
	rm myls.o myls

