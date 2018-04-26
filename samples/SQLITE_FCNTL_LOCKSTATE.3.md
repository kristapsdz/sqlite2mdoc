SQLITE\_FCNTL\_LOCKSTATE(3) - Library Functions Manual

# NAME

**SQLITE\_FCNTL\_LOCKSTATE**,
**SQLITE\_FCNTL\_GET\_LOCKPROXYFILE**,
**SQLITE\_FCNTL\_SET\_LOCKPROXYFILE**,
**SQLITE\_FCNTL\_LAST\_ERRNO**,
**SQLITE\_FCNTL\_SIZE\_HINT**,
**SQLITE\_FCNTL\_CHUNK\_SIZE**,
**SQLITE\_FCNTL\_FILE\_POINTER**,
**SQLITE\_FCNTL\_SYNC\_OMITTED**,
**SQLITE\_FCNTL\_WIN32\_AV\_RETRY**,
**SQLITE\_FCNTL\_PERSIST\_WAL**,
**SQLITE\_FCNTL\_OVERWRITE**,
**SQLITE\_FCNTL\_VFSNAME**,
**SQLITE\_FCNTL\_POWERSAFE\_OVERWRITE**,
**SQLITE\_FCNTL\_PRAGMA**,
**SQLITE\_FCNTL\_BUSYHANDLER**,
**SQLITE\_FCNTL\_TEMPFILENAME**,
**SQLITE\_FCNTL\_MMAP\_SIZE**,
**SQLITE\_FCNTL\_TRACE**,
**SQLITE\_FCNTL\_HAS\_MOVED**,
**SQLITE\_FCNTL\_SYNC**,
**SQLITE\_FCNTL\_COMMIT\_PHASETWO**,
**SQLITE\_FCNTL\_WIN32\_SET\_HANDLE**,
**SQLITE\_FCNTL\_WAL\_BLOCK**,
**SQLITE\_FCNTL\_ZIPVFS**,
**SQLITE\_FCNTL\_RBU**,
**SQLITE\_FCNTL\_VFS\_POINTER**,
**SQLITE\_FCNTL\_JOURNAL\_POINTER**,
**SQLITE\_FCNTL\_WIN32\_GET\_HANDLE**,
**SQLITE\_FCNTL\_PDB** - Standard File Control Opcodes

# SYNOPSIS

**#include &lt;sqlite.h>**

**#define SQLITE\_FCNTL\_LOCKSTATE**  
**#define SQLITE\_FCNTL\_GET\_LOCKPROXYFILE**  
**#define SQLITE\_FCNTL\_SET\_LOCKPROXYFILE**  
**#define SQLITE\_FCNTL\_LAST\_ERRNO**  
**#define SQLITE\_FCNTL\_SIZE\_HINT**  
**#define SQLITE\_FCNTL\_CHUNK\_SIZE**  
**#define SQLITE\_FCNTL\_FILE\_POINTER**  
**#define SQLITE\_FCNTL\_SYNC\_OMITTED**  
**#define SQLITE\_FCNTL\_WIN32\_AV\_RETRY**  
**#define SQLITE\_FCNTL\_PERSIST\_WAL**  
**#define SQLITE\_FCNTL\_OVERWRITE**  
**#define SQLITE\_FCNTL\_VFSNAME**  
**#define SQLITE\_FCNTL\_POWERSAFE\_OVERWRITE**  
**#define SQLITE\_FCNTL\_PRAGMA**  
**#define SQLITE\_FCNTL\_BUSYHANDLER**  
**#define SQLITE\_FCNTL\_TEMPFILENAME**  
**#define SQLITE\_FCNTL\_MMAP\_SIZE**  
**#define SQLITE\_FCNTL\_TRACE**  
**#define SQLITE\_FCNTL\_HAS\_MOVED**  
**#define SQLITE\_FCNTL\_SYNC**  
**#define SQLITE\_FCNTL\_COMMIT\_PHASETWO**  
**#define SQLITE\_FCNTL\_WIN32\_SET\_HANDLE**  
**#define SQLITE\_FCNTL\_WAL\_BLOCK**  
**#define SQLITE\_FCNTL\_ZIPVFS**  
**#define SQLITE\_FCNTL\_RBU**  
**#define SQLITE\_FCNTL\_VFS\_POINTER**  
**#define SQLITE\_FCNTL\_JOURNAL\_POINTER**  
**#define SQLITE\_FCNTL\_WIN32\_GET\_HANDLE**  
**#define SQLITE\_FCNTL\_PDB**

# DESCRIPTION

These integer constants are opcodes for the xFileControl method of
the sqlite3\_io\_methods object and for the sqlite3\_file\_control()
interface.

*	The SQLITE\_FCNTL\_LOCKSTATE opcode is used for
	debugging.
	This opcode causes the xFileControl method to write the current state
	of the lock (one of SQLITE\_LOCK\_NONE, SQLITE\_LOCK\_SHARED,
	SQLITE\_LOCK\_RESERVED, SQLITE\_LOCK\_PENDING,
	or SQLITE\_LOCK\_EXCLUSIVE) into an integer that
	the pArg argument points to.
	This capability is used during testing and is only available when the
	SQLITE\_TEST compile-time option is used.

*	The SQLITE\_FCNTL\_SIZE\_HINT opcode is used by
	SQLite to give the VFS layer a hint of how large the database file
	will grow to be during the current transaction.
	This hint is not guaranteed to be accurate but it is often close.
	The underlying VFS might choose to preallocate database file space
	based on this hint in order to help writes to the database file run
	faster.

*	The SQLITE\_FCNTL\_CHUNK\_SIZE opcode is used to
	request that the VFS extends and truncates the database file in chunks
	of a size specified by the user.
	The fourth argument to sqlite3\_file\_control()
	should point to an integer (type int) containing the new chunk-size
	to use for the nominated database.
	Allocating database file space in large chunks (say 1MB at a time),
	may reduce file-system fragmentation and improve performance on some
	systems.

*	The SQLITE\_FCNTL\_FILE\_POINTER opcode is used
	to obtain a pointer to the sqlite3\_file object associated
	with a particular database connection.
	See also SQLITE\_FCNTL\_JOURNAL\_POINTER.

*	The SQLITE\_FCNTL\_JOURNAL\_POINTER opcode
	is used to obtain a pointer to the sqlite3\_file object
	associated with the journal file (either the rollback journal
	or the write-ahead log) for a particular database connection.
	See also SQLITE\_FCNTL\_FILE\_POINTER.

*	No longer in use.

*	The SQLITE\_FCNTL\_SYNC opcode is generated internally
	by SQLite and sent to the VFS immediately before the xSync method is
	invoked on a database file descriptor.
	Or, if the xSync method is not invoked because the user has configured
	SQLite with PRAGMA synchronous=OFF it is invoked
	in place of the xSync method.
	In most cases, the pointer argument passed with this file-control is
	NULL.
	However, if the database file is being synced as part of a multi-database
	commit, the argument points to a nul-terminated string containing the
	transactions master-journal file name.
	VFSes that do not need this signal should silently ignore this opcode.
	Applications should not call sqlite3\_file\_control()
	with this opcode as doing so may disrupt the operation of the specialized
	VFSes that do require it.

*	The SQLITE\_FCNTL\_COMMIT\_PHASETWO opcode
	is generated internally by SQLite and sent to the VFS after a transaction
	has been committed immediately but before the database is unlocked.
	VFSes that do not need this signal should silently ignore this opcode.
	Applications should not call sqlite3\_file\_control()
	with this opcode as doing so may disrupt the operation of the specialized
	VFSes that do require it.

*	The SQLITE\_FCNTL\_WIN32\_AV\_RETRY opcode is
	used to configure automatic retry counts and intervals for certain
	disk I/O operations for the windows VFS in order to provide robustness
	in the presence of anti-virus programs.
	By default, the windows VFS will retry file read, file write, and file
	delete operations up to 10 times, with a delay of 25 milliseconds before
	the first retry and with the delay increasing by an additional 25 milliseconds
	with each subsequent retry.
	This opcode allows these two values (10 retries and 25 milliseconds
	of delay) to be adjusted.
	The values are changed for all database connections within the same
	process.
	The argument is a pointer to an array of two integers where the first
	integer is the new retry count and the second integer is the delay.
	If either integer is negative, then the setting is not changed but
	instead the prior value of that setting is written into the array entry,
	allowing the current retry settings to be interrogated.
	The zDbName parameter is ignored.

*	The SQLITE\_FCNTL\_PERSIST\_WAL opcode is used
	to set or query the persistent Write Ahead Log setting.
	By default, the auxiliary write ahead log and shared memory files used
	for transaction control are automatically deleted when the latest connection
	to the database closes.
	Setting persistent WAL mode causes those files to persist after close.
	Persisting the files is useful when other processes that do not have
	write permission on the directory containing the database file want
	to read the database file, as the WAL and shared memory files must
	exist in order for the database to be readable.
	The fourth parameter to sqlite3\_file\_control()
	for this opcode should be a pointer to an integer.
	That integer is 0 to disable persistent WAL mode or 1 to enable persistent
	WAL mode.
	If the integer is -1, then it is overwritten with the current WAL persistence
	setting.

*	The SQLITE\_FCNTL\_POWERSAFE\_OVERWRITE
	opcode is used to set or query the persistent "powersafe-overwrite"
	or "PSOW" setting.
	The PSOW setting determines the SQLITE\_IOCAP\_POWERSAFE\_OVERWRITE
	bit of the xDeviceCharacteristics methods.
	The fourth parameter to sqlite3\_file\_control()
	for this opcode should be a pointer to an integer.
	That integer is 0 to disable zero-damage mode or 1 to enable zero-damage
	mode.
	If the integer is -1, then it is overwritten with the current zero-damage
	mode setting.

*	The SQLITE\_FCNTL\_OVERWRITE opcode is invoked
	by SQLite after opening a write transaction to indicate that, unless
	it is rolled back for some reason, the entire database file will be
	overwritten by the current transaction.
	This is used by VACUUM operations.

*	The SQLITE\_FCNTL\_VFSNAME opcode can be used to
	obtain the names of all VFSes in the VFS stack.
	The names are of all VFS shims and the final bottom-level VFS are written
	into memory obtained from sqlite3\_malloc() and the
	result is stored in the char\* variable that the fourth parameter of
	sqlite3\_file\_control() points to.
	The caller is responsible for freeing the memory when done.
	As with all file-control actions, there is no guarantee that this will
	actually do anything.
	Callers should initialize the char\* variable to a NULL pointer in case
	this file-control is not implemented.
	This file-control is intended for diagnostic use only.

*	The SQLITE\_FCNTL\_VFS\_POINTER opcode finds a
	pointer to the top-level VFSes currently in use.
	The argument X in sqlite3\_file\_control(db,SQLITE\_FCNTL\_VFS\_POINTER,X)
	must be of type "sqlite3\_vfs \*\*".
	This opcodes will set \*X to a pointer to the top-level VFS.
	When there are multiple VFS shims in the stack, this opcode finds the
	upper-most shim only.

*	Whenever a PRAGMA statement is parsed, an SQLITE\_FCNTL\_PRAGMA
	file control is sent to the open sqlite3\_file object corresponding
	to the database file to which the pragma statement refers.
	The argument to the SQLITE\_FCNTL\_PRAGMA file control
	is an array of pointers to strings (char\*\*) in which the second element
	of the array is the name of the pragma and the third element is the
	argument to the pragma or NULL if the pragma has no argument.
	The handler for an SQLITE\_FCNTL\_PRAGMA file control
	can optionally make the first element of the char\*\* argument point
	to a string obtained from sqlite3\_mprintf() or the
	equivalent and that string will become the result of the pragma or
	the error message if the pragma fails.
	If the SQLITE\_FCNTL\_PRAGMA file control returns
	SQLITE\_NOTFOUND, then normal PRAGMA processing
	continues.
	If the SQLITE\_FCNTL\_PRAGMA file control returns
	SQLITE\_OK, then the parser assumes that the VFS has handled
	the PRAGMA itself and the parser generates a no-op prepared statement
	if result string is NULL, or that returns a copy of the result string
	if the string is non-NULL.
	If the SQLITE\_FCNTL\_PRAGMA file control returns
	any result code other than SQLITE\_OK or SQLITE\_NOTFOUND,
	that means that the VFS encountered an error while handling the PRAGMA
	and the compilation of the PRAGMA fails with an error.
	The SQLITE\_FCNTL\_PRAGMA file control occurs at the
	beginning of pragma statement analysis and so it is able to override
	built-in PRAGMA statements.

*	The SQLITE\_FCNTL\_BUSYHANDLER file-control may
	be invoked by SQLite on the database file handle shortly after it is
	opened in order to provide a custom VFS with access to the connections
	busy-handler callback.
	The argument is of type (void \*\*) - an array of two (void \*) values.
	The first (void \*) actually points to a function of type (int (\*)(void
	\*)).
	In order to invoke the connections busy-handler, this function should
	be invoked with the second (void \*) in the array as the only argument.
	If it returns non-zero, then the operation should be retried.
	If it returns zero, the custom VFS should abandon the current operation.

*	Application can invoke the SQLITE\_FCNTL\_TEMPFILENAME
	file-control to have SQLite generate a temporary filename using the
	same algorithm that is followed to generate temporary filenames for
	TEMP tables and other internal uses.
	The argument should be a char\*\* which will be filled with the filename
	written into memory obtained from sqlite3\_malloc().
	The caller should invoke sqlite3\_free() on the result
	to avoid a memory leak.

*	The SQLITE\_FCNTL\_MMAP\_SIZE file control is used
	to query or set the maximum number of bytes that will be used for memory-mapped
	I/O.
	The argument is a pointer to a value of type sqlite3\_int64 that is
	an advisory maximum number of bytes in the file to memory map.
	The pointer is overwritten with the old value.
	The limit is not changed if the value originally pointed to is negative,
	and so the current limit can be queried by passing in a pointer to
	a negative number.
	This file-control is used internally to implement PRAGMA mmap\_size.

*	The SQLITE\_FCNTL\_TRACE file control provides advisory
	information to the VFS about what the higher layers of the SQLite stack
	are doing.
	This file control is used by some VFS activity tracing shims.
	The argument is a zero-terminated string.
	Higher layers in the SQLite stack may generate instances of this file
	control if the SQLITE\_USE\_FCNTL\_TRACE compile-time
	option is enabled.

*	The SQLITE\_FCNTL\_HAS\_MOVED file control interprets
	its argument as a pointer to an integer and it writes a boolean into
	that integer depending on whether or not the file has been renamed,
	moved, or deleted since it was first opened.

*	The SQLITE\_FCNTL\_WIN32\_GET\_HANDLE opcode
	can be used to obtain the underlying native file handle associated
	with a file handle.
	This file control interprets its argument as a pointer to a native
	file handle and writes the resulting value there.

*	The SQLITE\_FCNTL\_WIN32\_SET\_HANDLE opcode
	is used for debugging.
	This opcode causes the xFileControl method to swap the file handle
	with the one pointed to by the pArg argument.
	This capability is used during testing and only needs to be supported
	when SQLITE\_TEST is defined.

*	The SQLITE\_FCNTL\_WAL\_BLOCK is a signal to the
	VFS layer that it might be advantageous to block on the next WAL lock
	if the lock is not immediately available.
	The WAL subsystem issues this signal during rare circumstances in order
	to fix a problem with priority inversion.
	Applications should **not** use this file-control.

*	The SQLITE\_FCNTL\_ZIPVFS opcode is implemented by
	zipvfs only.
	All other VFS should return SQLITE\_NOTFOUND for this opcode.

*	The SQLITE\_FCNTL\_RBU opcode is implemented by the special
	VFS used by the RBU extension only.
	All other VFS should return SQLITE\_NOTFOUND for this opcode.

# IMPLEMENTATION NOTES

These declarations were extracted from the
interface documentation at line 779.

	#define SQLITE_FCNTL_LOCKSTATE               1
	#define SQLITE_FCNTL_GET_LOCKPROXYFILE       2
	#define SQLITE_FCNTL_SET_LOCKPROXYFILE       3
	#define SQLITE_FCNTL_LAST_ERRNO              4
	#define SQLITE_FCNTL_SIZE_HINT               5
	#define SQLITE_FCNTL_CHUNK_SIZE              6
	#define SQLITE_FCNTL_FILE_POINTER            7
	#define SQLITE_FCNTL_SYNC_OMITTED            8
	#define SQLITE_FCNTL_WIN32_AV_RETRY          9
	#define SQLITE_FCNTL_PERSIST_WAL            10
	#define SQLITE_FCNTL_OVERWRITE              11
	#define SQLITE_FCNTL_VFSNAME                12
	#define SQLITE_FCNTL_POWERSAFE_OVERWRITE    13
	#define SQLITE_FCNTL_PRAGMA                 14
	#define SQLITE_FCNTL_BUSYHANDLER            15
	#define SQLITE_FCNTL_TEMPFILENAME           16
	#define SQLITE_FCNTL_MMAP_SIZE              18
	#define SQLITE_FCNTL_TRACE                  19
	#define SQLITE_FCNTL_HAS_MOVED              20
	#define SQLITE_FCNTL_SYNC                   21
	#define SQLITE_FCNTL_COMMIT_PHASETWO        22
	#define SQLITE_FCNTL_WIN32_SET_HANDLE       23
	#define SQLITE_FCNTL_WAL_BLOCK              24
	#define SQLITE_FCNTL_ZIPVFS                 25
	#define SQLITE_FCNTL_RBU                    26
	#define SQLITE_FCNTL_VFS_POINTER            27
	#define SQLITE_FCNTL_JOURNAL_POINTER        28
	#define SQLITE_FCNTL_WIN32_GET_HANDLE       29
	#define SQLITE_FCNTL_PDB                    30

# SEE ALSO

sqlite3\_file(3),
sqlite3\_file\_control(3),
sqlite3\_malloc(3),
sqlite3\_io\_methods(3),
sqlite3\_malloc(3),
sqlite3\_mprintf(3),
sqlite3\_vfs(3),
SQLITE\_FCNTL\_LOCKSTATE(3),
SQLITE\_IOCAP\_ATOMIC(3),
SQLITE\_LOCK\_NONE(3),
SQLITE\_OK(3)

OpenBSD 6.2 - April 26, 2018
