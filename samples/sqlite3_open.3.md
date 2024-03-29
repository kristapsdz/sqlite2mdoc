SQLITE3\_OPEN(3) - Library Functions Manual

# NAME

**sqlite3\_open**,
**sqlite3\_open16**,
**sqlite3\_open\_v2** - opening a new database connection

# SYNOPSIS

**#include &lt;sqlite3.h>**

*int*  
**sqlite3\_open**(*const char \*filename*,
*sqlite3 \*\*ppDb*);

*int*  
**sqlite3\_open16**(*const void \*filename*,
*sqlite3 \*\*ppDb*);

*int*  
**sqlite3\_open\_v2**(*const char \*filename*,
*sqlite3 \*\*ppDb*,
*int flags*,
*const char \*zVfs*);

# DESCRIPTION

These routines open an SQLite database file as specified by the filename
argument.
The filename argument is interpreted as UTF-8 for sqlite3\_open() and
sqlite3\_open\_v2() and as UTF-16 in the native byte order for sqlite3\_open16().
A database connection handle is usually returned
in \*ppDb, even if an error occurs.
The only exception is that if SQLite is unable to allocate memory to
hold the sqlite3 object, a NULL will be written into \*ppDb instead
of a pointer to the sqlite3 object.
If the database is opened (and/or created) successfully, then SQLITE\_OK
is returned.
Otherwise an error code is returned.
The
**sqlite3\_errmsg**()
or
**sqlite3\_errmsg16**()
routines can be used to obtain an English language description of the
error following a failure of any of the sqlite3\_open() routines.

The default encoding will be UTF-8 for databases created using sqlite3\_open()
or sqlite3\_open\_v2().
The default encoding for databases created using sqlite3\_open16() will
be UTF-16 in the native byte order.

Whether or not an error occurs when it is opened, resources associated
with the database connection handle should be released
by passing it to
**sqlite3\_close**()
when it is no longer required.

The sqlite3\_open\_v2() interface works like sqlite3\_open() except that
it accepts two additional parameters for additional control over the
new database connection.
The flags parameter to sqlite3\_open\_v2() must include, at a minimum,
one of the following three flag combinations:

SQLITE\_OPEN\_READONLY

> The database is opened in read-only mode.
> If the database does not already exist, an error is returned.

SQLITE\_OPEN\_READWRITE

> The database is opened for reading and writing if possible, or reading
> only if the file is write protected by the operating system.
> In either case the database must already exist, otherwise an error
> is returned.
> For historical reasons, if opening in read-write mode fails due to
> OS-level permissions, an attempt is made to open it in read-only mode.
> **sqlite3\_db\_readonly**()
> can be used to determine whether the database is actually read-write.

SQLITE\_OPEN\_READWRITE | SQLITE\_OPEN\_CREATE

> The database is opened for reading and writing, and is created if it
> does not already exist.
> This is the behavior that is always used for sqlite3\_open() and sqlite3\_open16().

In addition to the required flags, the following optional flags are
also supported:

SQLITE\_OPEN\_URI

> The filename can be interpreted as a URI if this flag is set.

SQLITE\_OPEN\_MEMORY

> The database will be opened as an in-memory database.
> The database is named by the "filename" argument for the purposes of
> cache-sharing, if shared cache mode is enabled, but the "filename"
> is otherwise ignored.

SQLITE\_OPEN\_NOMUTEX

> The new database connection will use the "multi-thread" threading mode.
> This means that separate threads are allowed to use SQLite at the same
> time, as long as each thread is using a different database connection.

SQLITE\_OPEN\_FULLMUTEX

> The new database connection will use the "serialized" threading mode.
> This means the multiple threads can safely attempt to use the same
> database connection at the same time.
> (Mutexes will block any actual concurrency, but in this mode there
> is no harm in trying.)

SQLITE\_OPEN\_SHAREDCACHE

> The database is opened shared cache enabled, overriding
> the default shared cache setting provided by
> **sqlite3\_enable\_shared\_cache**().
> The use of shared cache mode is discouraged
> and hence shared cache capabilities may be omitted from many builds
> of SQLite.
> In such cases, this option is a no-op.

SQLITE\_OPEN\_PRIVATECACHE

> The database is opened shared cache disabled, overriding
> the default shared cache setting provided by
> **sqlite3\_enable\_shared\_cache**().

SQLITE\_OPEN\_EXRESCODE

> The database connection comes up in "extended result code mode".
> In other words, the database behaves has if sqlite3\_extended\_result\_codes(db,1)
> where called on the database connection as soon as the connection is
> created.
> In addition to setting the extended result code mode, this flag also
> causes
> **sqlite3\_open\_v2**()
> to return an extended result code.

SQLITE\_OPEN\_NOFOLLOW

> The database filename is not allowed to contain a symbolic link

If the 3rd parameter to sqlite3\_open\_v2() is not one of the required
combinations shown above optionally combined with other SQLITE\_OPEN\_\* bits
then the behavior is undefined.
Historic versions of SQLite have silently ignored surplus bits in the
flags parameter to sqlite3\_open\_v2(), however that behavior might not
be carried through into future versions of SQLite and so applications
should not rely upon it.
Note in particular that the SQLITE\_OPEN\_EXCLUSIVE flag is a no-op for
sqlite3\_open\_v2().
The SQLITE\_OPEN\_EXCLUSIVE does \*not\* cause the open to fail if the
database already exists.
The SQLITE\_OPEN\_EXCLUSIVE flag is intended for use by the VFS interface
only, and not by sqlite3\_open\_v2().

The fourth parameter to sqlite3\_open\_v2() is the name of the sqlite3\_vfs
object that defines the operating system interface that the new database
connection should use.
If the fourth parameter is a NULL pointer then the default sqlite3\_vfs
object is used.

If the filename is ":memory:", then a private, temporary in-memory
database is created for the connection.
This in-memory database will vanish when the database connection is
closed.
Future versions of SQLite might make use of additional special filenames
that begin with the ":" character.
It is recommended that when a database filename actually does begin
with a ":" character you should prefix the filename with a pathname
such as "./" to avoid ambiguity.

If the filename is an empty string, then a private, temporary on-disk
database will be created.
This private database will be automatically deleted as soon as the
database connection is closed.

## URI Filenames

If URI filename interpretation is enabled, and the filename
argument begins with "file:", then the filename is interpreted as a
URI.
URI filename interpretation is enabled if the SQLITE\_OPEN\_URI
flag is set in the third argument to sqlite3\_open\_v2(), or if it has
been enabled globally using the SQLITE\_CONFIG\_URI
option with the
**sqlite3\_config**()
method or by the SQLITE\_USE\_URI compile-time option.
URI filename interpretation is turned off by default, but future releases
of SQLite might enable URI filename interpretation by default.
See "URI filenames" for additional information.

URI filenames are parsed according to RFC 3986.
If the URI contains an authority, then it must be either an empty string
or the string "localhost".
If the authority is not an empty string or "localhost", an error is
returned to the caller.
The fragment component of a URI, if present, is ignored.

SQLite uses the path component of the URI as the name of the disk file
which contains the database.
If the path begins with a '/' character, then it is interpreted as
an absolute path.
If the path does not begin with a '/' (meaning that the authority section
is omitted from the URI) then the path is interpreted as a relative
path.
On windows, the first component of an absolute path is a drive specification
(e.g. "C:").

The query component of a URI may contain parameters that are interpreted
either by SQLite itself, or by a custom VFS implementation.
SQLite and its built-in VFSes interpret the following query parameters:

*	**vfs**: The "vfs" parameter may be used to specify the name of a VFS object
	that provides the operating system interface that should be used to
	access the database file on disk.
	If this option is set to an empty string the default VFS object is
	used.
	Specifying an unknown VFS is an error.
	If sqlite3\_open\_v2() is used and the vfs option is present, then the
	VFS specified by the option takes precedence over the value passed
	as the fourth parameter to sqlite3\_open\_v2().

*	**mode**: The mode parameter may be set to either "ro", "rw", "rwc", or
	"memory".
	Attempting to set it to any other value is an error.
	If "ro" is specified, then the database is opened for read-only access,
	just as if the SQLITE\_OPEN\_READONLY flag had been
	set in the third argument to sqlite3\_open\_v2().
	If the mode option is set to "rw", then the database is opened for
	read-write (but not create) access, as if SQLITE\_OPEN\_READWRITE (but
	not SQLITE\_OPEN\_CREATE) had been set.
	Value "rwc" is equivalent to setting both SQLITE\_OPEN\_READWRITE and
	SQLITE\_OPEN\_CREATE.
	If the mode option is set to "memory" then a pure in-memory database
	that never reads or writes from disk is used.
	It is an error to specify a value for the mode parameter that is less
	restrictive than that specified by the flags passed in the third parameter
	to sqlite3\_open\_v2().

*	**cache**: The cache parameter may be set to either "shared" or "private".
	Setting it to "shared" is equivalent to setting the SQLITE\_OPEN\_SHAREDCACHE
	bit in the flags argument passed to sqlite3\_open\_v2().
	Setting the cache parameter to "private" is equivalent to setting the
	SQLITE\_OPEN\_PRIVATECACHE bit.
	If sqlite3\_open\_v2() is used and the "cache" parameter is present in
	a URI filename, its value overrides any behavior requested by setting
	SQLITE\_OPEN\_PRIVATECACHE or SQLITE\_OPEN\_SHAREDCACHE flag.

*	**psow**: The psow parameter indicates whether or not the powersafe overwrite
	property does or does not apply to the storage media on which the database
	file resides.

*	**nolock**: The nolock parameter is a boolean query parameter which if
	set disables file locking in rollback journal modes.
	This is useful for accessing a database on a filesystem that does not
	support locking.
	Caution:  Database corruption might result if two or more processes
	write to the same database and any one of those processes uses nolock=1.

*	**immutable**: The immutable parameter is a boolean query parameter that
	indicates that the database file is stored on read-only media.
	When immutable is set, SQLite assumes that the database file cannot
	be changed, even by a process with higher privilege, and so the database
	is opened read-only and all locking and change detection is disabled.
	Caution: Setting the immutable property on a database file that does
	in fact change can result in incorrect query results and/or SQLITE\_CORRUPT
	errors.

Specifying an unknown parameter in the query component of a URI is
not an error.
Future versions of SQLite might understand additional query parameters.
See "query parameters with special meaning to SQLite"
for additional information.

## URI filename examples

   URI filenames   Results
   file:data.db   Open the file "data.db" in the current directory.
   file:/home/fred/data.db  file:///home/fred/data.db   file://localhost/home/fred/data.db
  Open the database file "/home/fred/data.db".
   file://darkstar/home/fred/data.db   An error.
"darkstar" is not a recognized authority.
   file:///C:/Documents%20and%20Settings/fred/Desktop/data.db   Windows
only: Open the file "data.db" on fred's desktop on drive C:.
Note that the %20 escaping in this example is not strictly necessary
\- space characters can be used literally in URI filenames.
   file:data.db?mode=ro&cache=private   Open file "data.db" in the current
directory for read-only access.
Regardless of whether or not shared-cache mode is enabled by default,
use a private cache.
   file:/home/fred/data.db?vfs=unix-dotfile   Open file "/home/fred/data.db".
Use the special VFS "unix-dotfile" that uses dot-files in place of
posix advisory locking.
   file:data.db?mode=readonly   An error.
"readonly" is not a valid option for the "mode" parameter.
Use "ro" instead:  "file:data.db?mode=ro".

URI hexadecimal escape sequences (%HH) are supported within the path
and query components of a URI.
A hexadecimal escape sequence consists of a percent sign - "%" - followed
by exactly two hexadecimal digits specifying an octet value.
Before the path or query components of a URI filename are interpreted,
they are encoded using UTF-8 and all hexadecimal escape sequences replaced
by a single byte containing the corresponding octet.
If this process generates an invalid UTF-8 encoding, the results are
undefined.

**Note to Windows users:**  The encoding used for the filename argument
of sqlite3\_open() and sqlite3\_open\_v2() must be UTF-8, not whatever
codepage is currently defined.
Filenames containing international characters must be converted to
UTF-8 prior to passing them into sqlite3\_open() or sqlite3\_open\_v2().

**Note to Windows Runtime users:**  The temporary directory must be set
prior to calling sqlite3\_open() or sqlite3\_open\_v2().
Otherwise, various features that require the use of temporary files
may fail.

# IMPLEMENTATION NOTES

These declarations were extracted from the
interface documentation at line 3458.

	SQLITE_API int sqlite3_open(
	  const char *filename,   /* Database filename (UTF-8) */
	  sqlite3 **ppDb          /* OUT: SQLite db handle */
	);
	SQLITE_API int sqlite3_open16(
	  const void *filename,   /* Database filename (UTF-16) */
	  sqlite3 **ppDb          /* OUT: SQLite db handle */
	);
	SQLITE_API int sqlite3_open_v2(
	  const char *filename,   /* Database filename (UTF-8) */
	  sqlite3 **ppDb,         /* OUT: SQLite db handle */
	  int flags,              /* Flags */
	  const char *zVfs        /* Name of VFS module to use */
	);

# SEE ALSO

sqlite3(3),
sqlite3\_close(3),
sqlite3\_config(3),
sqlite3\_db\_readonly(3),
sqlite3\_enable\_shared\_cache(3),
sqlite3\_errcode(3),
sqlite3\_temp\_directory(3),
sqlite3\_vfs(3),
SQLITE\_CONFIG\_SINGLETHREAD(3),
SQLITE\_IOCAP\_ATOMIC(3),
SQLITE\_OK(3),
SQLITE\_OPEN\_READONLY(3)

OpenBSD 7.2 - May 9, 2023
