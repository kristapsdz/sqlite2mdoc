## Synopsis

This utility accepts an [SQLite](https://www.sqlite.org) header file
`sqlite.h` and produces a set of decently well-formed
[mdoc(7)](http://man.openbsd.org/OpenBSD-current/man7/mdoc.7) files
documenting the C API.
These will be roughly equivalent to the [C-language Interface
Specification for SQLite](https://www.sqlite.org/c3ref/intro.html).

See the [sqlite2mdoc.1](sqlite2mdoc.1) manpage for details.

This [GitHub](https://www.github.com) repository is a read-only mirror
of the project's CVS repository.

## Installation

Simply run `make` and `make install`.
There are no dependencies

This software has only been tested on OpenBSD and Mac OS X machines.
Porting it won't take much effort.

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
