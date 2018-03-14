include Makefile.configure

VERSION		 = 0.1.0

sqlite2mdoc: main.o
	$(CC) -o $@ main.o $(LDADD)

main.o: config.h

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/man/man1
	install -m 0755 sqlite2mdoc $(DESTDIR)$(PREFIX)/bin
	install -m 0444 sqlite2mdoc.1 $(DESTDIR)$(PREFIX)/man/man1

clean:
	rm -f sqlite2mdoc main.o
