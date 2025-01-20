/*
 * Copyright (c) Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#if HAVE_SYS_QUEUE
# include <sys/queue.h>
#endif
#include <assert.h>
#include <ctype.h>
#if HAVE_ERR
# include <err.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

static	const char *const preprocs[PREPROC__MAX] = {
	"SQLITE_API", /* PREPROC_SQLITE_API */
	"SQLITE_DEPRECATED", /* PREPROC_SQLITE_DEPRECATED */
	"SQLITE_EXPERIMENTAL", /* PREPROC_SQLITE_EXPERIMENTAL */
	"SQLITE_EXTERN", /* PREPROC_SQLITE_EXTERN */
	"SQLITE_STDCALL", /* PREPROC_SQLITE_STDCALL */
};

void
print_synopsis(FILE *f, const struct decl *first, const struct defn *d)
{
	size_t		 sz, i, ns, fnsz;
	char		*cp;
	const char	*args, *str, *end, *fn;
	enum preproc	 pre;

	if (first->type != DECLTYPE_CPP &&
	    first->type != DECLTYPE_C)
		return;

	/* Easy: just print the CPP name. */

	if (first->type == DECLTYPE_CPP) {
		fprintf(f, ".Fd #define %s\n",
			first->text);
		return;
	}

	/* First, strip out the sqlite CPPs. */

	for (i = 0; i < first->textsz; ) {
		for (pre = 0; pre < PREPROC__MAX; pre++) {
			sz = strlen(preprocs[pre]);
			if (strncmp(preprocs[pre],
			    &first->text[i], sz))
				continue;
			i += sz;
			while (isspace((unsigned char)first->text[i]))
				i++;
			break;
		}
		if (pre == PREPROC__MAX)
			break;
	}

	/* If we're a typedef, immediately print Vt. */

	if (strncmp(&first->text[i], "typedef", 7) == 0) {
		fprintf(f, ".Vt %s\n", &first->text[i]);
		return;
	}

	/* Are we a struct? */

	if (first->textsz > 2 &&
	    first->text[first->textsz - 2] == '}' &&
	    (cp = strchr(&first->text[i], '{')) != NULL) {
		*cp = '\0';
		fprintf(f, ".Vt %s;\n", &first->text[i]);
		/* Restore brace for later usage. */
		*cp = '{';
		return;
	}

	/* Catch remaining non-functions. */

	if (first->textsz > 2 &&
	    first->text[first->textsz - 2] != ')') {
		fprintf(f, ".Vt %s\n", &first->text[i]);
		return;
	}

	str = &first->text[i];
	if ((args = strchr(str, '(')) == NULL || args == str) {
		/* What is this? */
		fputs(".Bd -literal\n", f);
		fputs(&first->text[i], f);
		fputs("\n.Ed\n", f);
		return;
	}

	/*
	 * Current state:
	 *  type_t *function      (args...)
	 *  ^str                  ^args
	 * Scroll back to end of function name.
	 */

	end = args - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	/*
	 * Current state:
	 *  type_t *function      (args...)
	 *  ^str           ^end   ^args
	 * Scroll back to what comes before.
	 */

	for (fnsz = 0; end > str; end--, fnsz++)
		if (isspace((unsigned char)*end) || *end == '*')
			break;

	if (fnsz == 0)
		warnx("%s:%zu: zero-length "
			"function name", d->fn, d->ln);
	fn = end + 1;

	/*
	 * Current state:
	 *  type_t *function      (args...)
	 *  ^str   ^end           ^args
	 *  type_t  function      (args...)
	 *  ^str   ^end           ^args
	 * Strip away whitespace.
	 */

	while (end > str && isspace((unsigned char)*end))
		end--;

	/*
	 * type_t *function      (args...)
	 * ^str   ^end           ^args
	 * type_t  function      (args...)
	 * ^str ^end             ^args
	 */

	/*
	 * If we can't find what came before, then the function
	 * has no type, which is odd... let's just call it void.
	 */

	if (end > str) {
		fprintf(f, ".Ft %.*s\n",
			(int)(end - str + 1), str);
		fprintf(f, ".Fo %.*s\n", (int)fnsz, fn);
	} else {
		fputs(".Ft void\n", f);
		fprintf(f, ".Fo %.*s\n", (int)fnsz, fn);
	}

	/*
	 * Convert function arguments into `Fa' clauses.
	 * This also handles nested function pointers, which
	 * would otherwise throw off the delimeters.
	 */

	for (;;) {
		str = ++args;
		while (isspace((unsigned char)*str))
			str++;
		fputs(".Fa \"", f);
		ns = 0;
		while (*str != '\0' &&
		       (ns || *str != ',') &&
		       (ns || *str != ')')) {
			/*
			 * Handle comments in the declarations.
			 */
			if (str[0] == '/' && str[1] == '*') {
				str += 2;
				for ( ; str[0] != '\0'; str++)
					if (str[0] == '*' && str[1] == '/')
						break;
				if (*str == '\0')
					break;
				str += 2;
				while (isspace((unsigned char)*str))
					str++;
				if (*str == '\0' ||
				    (ns == 0 && *str == ',') ||
				    (ns == 0 && *str == ')'))
					break;
			}
			if (*str == '(')
				ns++;
			else if (*str == ')')
				ns--;

			/*
			 * Handle some instances of whitespace
			 * by compressing it down.
			 * However, if the whitespace ends at
			 * the end-of-definition, then don't
			 * print it at all.
			 */

			if (isspace((unsigned char)*str)) {
				while (isspace((unsigned char)*str))
					str++;
				/* Are we at a comment? */
				if (str[0] == '/' && str[1] == '*')
					continue;
				if (*str == '\0' ||
				    (ns == 0 && *str == ',') ||
				    (ns == 0 && *str == ')'))
					break;
				fputc(' ', f);
			} else {
				fputc(*str, f);
				str++;
			}
		}
		fputs("\"\n", f);
		if (*str == '\0' || *str == ')')
			break;
		args = str;
	}

	fputs(".Fc\n", f);
}
