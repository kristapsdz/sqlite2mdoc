## Synopsis

This utility accepts an [SQLitee](https://www.sqlite.org) header file
`sqlite3.h` and produces a set of decently well-formed
[mdoc(7)](https://man.openbsd.org/OpenBSD-current/man7/mdoc.7) files
documenting the C API.
These will be roughly equivalent to the [C-language Interface
Specification for SQLite](https://www.sqlite.org/c3ref/intro.html).

You can also use it for any file(s) using the documentation standards of
SQLite.
See the [sqlite2mdoc.1](sqlite2mdoc.1) manpage for syntax details.

This [GitHub](https://www.github.com) repository is a read-only mirror
of the project's CVS repository.

**Note**: this only works with sqlite3, *not* the original `sqlite.h`
format.

## Installation

Run `./configure` then `make`.

This utility isn't meant for installation, but for integration into your
SQLite deployment phase.  You can run `make install`, however, if you
plan on using it for other documentation.

There are no compile-time or run-time dependencies.

This software has been used on OpenBSD, Mac OS X, and Linux machines.

## Examples

I've used [mandoc](https://mandoc.bsd.lv) to generate some Markdown from
the [mdoc(7)](https://man.openbsd.org/mdoc.7) output.

- [sqlite3\_open(3)](samples/sample3_open.3.md)
- [SQLITE\_FCNTL\_LOCKSTATE(3)](samples/SQLITE_FCNTL_LOCKSTATE.3.md)

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
