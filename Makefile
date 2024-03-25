CC=gcc
CFLAGS=-Wall -pedantic -std=gnu99 -I /local/courses/csse2310/include -L /local/courses/csse2310/lib -l csse2310a3 -l csse2310a4 -l stringstore
LIBCFLAGS=-fPIC -Wall -pedantic -std=gnu99 -I /local/courses/csse2310/include
SERVERFLAGS = -pthread
.DEFAULT_GOAL := all
all: dbclient dbserver libstringstore.so

dbclient: dbclient.o http.o
	$(CC) $(CFLAGS) $^ -g -o $@
dbserver: dbserver.o http.o
	$(CC) $(CFLAGS) $(SERVERFLAGS) $^ -g -o $@
# Turn stringstore.o into shared library libstringstore.so
libstringstore.so: stringstore.o
	$(CC) -shared -o $@ stringstore.o

# Compile source files to objects
dbclient.o: dbclient.c dbclient.h
dbserver.o: dbserver.c dbserver.h
http.o: http.c http.h
stringstore.o: stringstore.c
	$(CC) $(LIBCFLAGS) -c $<
clean:
	rm -f dbclient dbserver *.o *.so
