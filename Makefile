.PHONY: distclean clean regress distcheck

include Makefile.configure

WWWDIR		 = /var/www/vhosts/kristaps.bsd.lv/htdocs/sqlite2mdoc
VERSION		 = 0.1.9
DOTAR 		 = Makefile \
		   compats.c \
		   main.c \
		   tests.c \
		   sqlite2mdoc.1

all: sqlite2mdoc

sqlite2mdoc: main.o compats.o
	$(CC) -o $@ main.o compats.o $(LDFLAGS) $(LDADD)

www: sqlite2mdoc.tar.gz sqlite2mdoc.tar.gz.sha512

installwww: www
	mkdir -p $(WWWDIR)/snapshots
	$(INSTALL_DATA) sqlite2mdoc.tar.gz $(WWWDIR)/snapshots
	$(INSTALL_DATA) sqlite2mdoc.tar.gz $(WWWDIR)/snapshots/sqlite2mdoc-$(VERSION).tar.gz

sqlite2mdoc.tar.gz:
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/regress
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/regress/expect
	$(INSTALL) -m 0644 $(DOTAR) .dist/sqlite2mdoc-$(VERSION)
	$(INSTALL) -m 0644 regress/sqlite3.h .dist/sqlite2mdoc-$(VERSION)/regress
	$(INSTALL) -m 0644 regress/expect/*.3 .dist/sqlite2mdoc-$(VERSION)/regress/expect
	$(INSTALL) -m 0755 configure .dist/sqlite2mdoc-$(VERSION)
	( cd .dist/ && tar zcf ../$@ sqlite2mdoc-$(VERSION) )
	rm -rf .dist/

sqlite2mdoc.tar.gz.sha512: sqlite2mdoc.tar.gz
	openssl dgst -sha512 -hex sqlite2mdoc.tar.gz >$@

main.o: config.h

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_PROGRAM) sqlite2mdoc $(DESTDIR)$(BINDIR)
	$(INSTALL_MAN) sqlite2mdoc.1 $(DESTDIR)$(MANDIR)/man1

distcheck: sqlite2mdoc.tar.gz sqlite2mdoc.tar.gz.sha512
	mandoc -Tlint -Werror sqlite2mdoc.1
	rm -rf .distcheck
	[ "`openssl dgst -sha512 -hex sqlite2mdoc.tar.gz`" = "`cat sqlite2mdoc.tar.gz.sha512`" ] || \
 		{ echo "Checksum does not match." 1>&2 ; exit 1 ; }
	mkdir -p .distcheck
	( cd .distcheck && tar -zvxpf ../sqlite2mdoc.tar.gz )
	( cd .distcheck/sqlite2mdoc-$(VERSION) && ./configure PREFIX=prefix )
	( cd .distcheck/sqlite2mdoc-$(VERSION) && $(MAKE) )
	( cd .distcheck/sqlite2mdoc-$(VERSION) && $(MAKE) regress )
	( cd .distcheck/sqlite2mdoc-$(VERSION) && $(MAKE) install )
	rm -rf .distcheck

distclean: clean
	rm -f config.h config.log Makefile.configure

regen_regress: all
	mkdir -p regress/expect/tmp
	./sqlite2mdoc -p regress/expect/tmp regress/sqlite3.h
	@for f in regress/expect/tmp/*.3 ; do \
		bn=`basename $$f` ; \
		echo $$bn ; \
		sed 1d $$f > regress/expect/$$bn.tmp ; \
		set +e ; \
		cmp regress/expect/$$bn.tmp regress/expect/$$bn >/dev/null 2>&1 ; \
		if [ $$? -ne 0 ] ; then \
			diff -u regress/expect/$$bn regress/expect/$$bn.tmp ; \
			mv -f regress/expect/$$bn.tmp regress/expect/$$bn ; \
		fi ; \
		set -e ; \
	done
	rm -rf regress/expect/tmp

regress: all
	rm -rf regress/out
	mkdir regress/out
	./sqlite2mdoc -p regress/out regress/sqlite3.h
	@for f in regress/out/*.3 ; do \
		sed 1d $$f > $$f.tmp ; \
		mv -f $$f.tmp $$f ; \
	done
	@for f in regress/out/*.3 ; do \
		echo diff $$f regress/expect/`basename $$f` ; \
		diff -u $$f regress/expect/`basename $$f` ; \
	done
	@for f in regress/expect/*.3 ; do \
		echo diff $$f regress/out/`basename $$f` ; \
		diff -u $$f regress/out/`basename $$f` ; \
	done
	rm -rf regress/out

clean:
	rm -f sqlite2mdoc main.o compats.o sqlite2mdoc.tar.gz sqlite2mdoc.tar.gz.sha512
	rm -rf regress/out
