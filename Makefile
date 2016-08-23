CC = gcc
CFLAGS = -Wall

INCLUDES = -I/usr/include/cairo -I/usr/include/swfdec-0.8 -I/usr/include/glib-2.0 \
	-I/usr/lib/glib-2.0/include

LIBS = -lcairo -lswfdec-0.8 -lgobject-2.0

swf2pdf: swf2pdf.c
	$(CC) $(CFLAGS) $(INCLUDES) swf2pdf.c -o swf2pdf $(LIBS)
