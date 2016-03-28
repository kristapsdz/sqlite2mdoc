CFLAGS	+= -g -W -Wall

sqlite2mdoc: main.o
	$(CC) -o $@ main.o

clean:
	rm -f sqlite2mdoc main.o
	rm -rf sqlite2mdoc.dSYM
