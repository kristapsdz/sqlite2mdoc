.PHONY: distclean clean regress distcheck

include Makefile.configure
WWWDIR		 = /var/www/vhosts/kristaps.bsd.lv/htdocs/sqlite2mdoc
sinclude Makefile.local
VERSION		 = 1.0.0
DOTAR 		 = Makefile \
		   compats.c \
		   main.c \
		   tests.c \
		   sqlite2mdoc.1
VALGRIND_ARGS	 = -q --leak-check=full --leak-resolution=high --show-reachable=yes

all: sqlite2mdoc

sqlite2mdoc: main.o compats.o
	$(CC) -o $@ main.o compats.o $(LDFLAGS) $(LDADD)

www: sqlite2mdoc.tar.gz sqlite2mdoc.tar.gz.sha512

installwww: www
	mkdir -p $(WWWDIR)/snapshots
	$(INSTALL_DATA) sqlite2mdoc.tar.gz $(WWWDIR)/snapshots
	$(INSTALL_DATA) sqlite2mdoc.tar.gz.sha512 $(WWWDIR)/snapshots
	$(INSTALL_DATA) sqlite2mdoc.tar.gz $(WWWDIR)/snapshots/sqlite2mdoc-$(VERSION).tar.gz
	$(INSTALL_DATA) sqlite2mdoc.tar.gz.sha512 $(WWWDIR)/snapshots/sqlite2mdoc-$(VERSION).sha512

sqlite2mdoc.tar.gz:
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/regress
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/regress/expect-3.29.0
	mkdir -p .dist/sqlite2mdoc-$(VERSION)/regress/expect-3.42.0
	$(INSTALL) -m 0644 $(DOTAR) .dist/sqlite2mdoc-$(VERSION)
	$(INSTALL) -m 0644 regress/sqlite3-3.29.0.h .dist/sqlite2mdoc-$(VERSION)/regress
	$(INSTALL) -m 0644 regress/sqlite3-3.42.0.h .dist/sqlite2mdoc-$(VERSION)/regress
	$(INSTALL) -m 0644 regress/expect-3.29.0/*.3 .dist/sqlite2mdoc-$(VERSION)/regress/expect-3.29.0
	$(INSTALL) -m 0644 regress/expect-3.42.0/*.3 .dist/sqlite2mdoc-$(VERSION)/regress/expect-3.42.0
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

valgrind: all
	@for f in regress/*.h ; do \
		echo "valgrind $(VALGRIND_ARGS) ./sqlite2mdoc -n $$f" ; \
		valgrind $(VALGRIND_ARGS) ./sqlite2mdoc -n $$f >/dev/null ; \
	done

regen_regress: all
	@for f in regress/*.h ; do \
		ver=`basename $$f .h | sed -e 's!sqlite3-!!'` ; \
		mkdir -p regress/expect-$$ver/tmp ; \
		./sqlite2mdoc -p regress/expect-$$ver/tmp regress/sqlite3-$$ver.h ; \
		for f in regress/expect-$$ver/tmp/*.3 ; do \
			bn=`basename $$f` ; \
			echo $$bn ; \
			sed 1d $$f > regress/expect-$$ver/$$bn.tmp ; \
			set +e ; \
			if [ -f regress/expect-$$ver/$$bn ]; then \
				cmp regress/expect-$$ver/$$bn.tmp regress/expect-$$ver/$$bn >/dev/null 2>&1 ; \
				if [ $$? -ne 0 ] ; then \
					diff -u regress/expect-$$ver/$$bn regress/expect-$$ver/$$bn.tmp ; \
				fi ; \
			fi ; \
			mv -f regress/expect-$$ver/$$bn.tmp regress/expect-$$ver/$$bn ; \
			set -e ; \
		done ; \
		rm -rf regress/expect-$$ver/tmp ; \
	done

regress: all
	@for f in regress/*.h ; do \
		ver=`basename $$f .h | sed -e 's!sqlite3-!!'` ; \
		rm -rf regress/out ; \
		mkdir -p regress/out ; \
		./sqlite2mdoc -p regress/out $$f ; \
		for f in regress/out/*.3 ; do \
			sed 1d $$f > $$f.tmp ; \
			mv -f $$f.tmp $$f ; \
		done ; \
		for f in regress/out/*.3 ; do \
			echo diff $$f regress/expect-$$ver/`basename $$f` ; \
			diff -u $$f regress/expect-$$ver/`basename $$f` ; \
		done ; \
		for f in regress/expect-$$ver/*.3 ; do \
			echo diff $$f regress/out/`basename $$f` ; \
			diff -u $$f regress/out/`basename $$f` ; \
		done ; \
	done
	rm -rf regress/out

clean:
	rm -f sqlite2mdoc main.o compats.o sqlite2mdoc.tar.gz sqlite2mdoc.tar.gz.sha512
	rm -rf regress/out
