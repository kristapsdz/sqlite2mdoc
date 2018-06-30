.PHONY: distclean clean

include Makefile.configure

WWWDIR		 = /var/www/vhosts/kristaps.bsd.lv/htdocs/sqlite2mdoc
VERSION		 = 0.1.3
DOTAR 		 = Makefile \
		   compats.c \
		   main.c \
		   tests.c \
		   sqlite2mdoc.1

all: sqlite2mdoc

sqlite2mdoc: main.o compats.o
	$(CC) -o $@ main.o compats.o $(LDFLAGS) $(LDADD)

www: sqlite2mdoc.tar.gz

installwww: www
	mkdir -p $(WWWDIR)/snapshots
	install -m 0444 sqlite2mdoc.tar.gz $(WWWDIR)/snapshots
	install -m 0444 sqlite2mdoc.tar.gz $(WWWDIR)/snapshots/sqlite2mdoc-$(VERSION).tar.gz

sqlite2mdoc.tar.gz:
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/
	install -m 0644 $(DOTAR) .dist/sqlite2mdoc-$(VERSION)
	install -m 0755 configure .dist/sqlite2mdoc-$(VERSION)
	( cd .dist/ && tar zcf ../$@ ./ )
	rm -rf .dist/

main.o: config.h

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/man/man1
	install -m 0755 sqlite2mdoc $(DESTDIR)$(PREFIX)/bin
	install -m 0444 sqlite2mdoc.1 $(DESTDIR)$(PREFIX)/man/man1

distclean: clean
	rm -f config.h config.log Makefile.configure

clean:
	rm -f sqlite2mdoc main.o compats.o sqlite2mdoc.tar.gz
