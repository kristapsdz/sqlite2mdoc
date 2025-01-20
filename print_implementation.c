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
#include <search.h>
#include <stdint.h> /* uintptr_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

/*
 * Convenience function to look up which manpage "hosts" a certain
 * keyword.  For example, SQLITE_OK(3) also handles SQLITE_TOOBIG and so
 * on, so a reference to SQLITE_TOOBIG should actually point to
 * SQLITE_OK.
 * Returns the keyword's file if found or NULL.
 */
static const char *
lookup(const char *key)
{
	ENTRY			 ent;
	ENTRY			*res;
	const struct defn	*d;

	ent.key = (char *)(uintptr_t)key;
	ent.data = NULL;

	if ((res = hsearch(ent, FIND)) == NULL)
		return NULL;

	d = (const struct defn *)res->data;
	if (d->nmsz == 0)
		return NULL;

	assert(d->nms[0] != NULL);
	return d->nms[0];
}

static int
xrcmp(const void *p1, const void *p2)
{
	const char	*s1 = lookup(*(const char **)(uintptr_t)p1),
	      		*s2 = lookup(*(const char **)(uintptr_t)p2);

	/* Silence bogus warnings about un-consting. */


	if (s1 == NULL)
		s1 = "";
	if (s2 == NULL)
		s2 = "";

	return strcasecmp(s1, s2);
}

void
print_implementation(FILE *f, const struct defn *d, int verbose)
{
	size_t		 i, last;
	const char	*res, *lastres;

	fprintf(f, "These declarations were extracted from the\n"
	      "interface documentation at line %zu.\n", d->ln);
	fputs(".Bd -literal\n", f);
	fputs(d->fulldesc, f);
	fputs(".Ed\n", f);

	/*
	 * Look up all of our keywords (which are in the xrs field) in
	 * the table of all known keywords.
	 * Don't print duplicates.
	 */

	if (d->xrsz == 0)
		return;
	qsort(d->xrs, d->xrsz, sizeof(char *), xrcmp);
	lastres = NULL;
	for (last = 0, i = 0; i < d->xrsz; i++) {
		res = lookup(d->xrs[i]);

		/* Ignore self-reference. */

		if (res == d->nms[0] && verbose)
			warnx("%s:%zu: self-reference: %s",
				d->fn, d->ln, d->xrs[i]);
		if (res == d->nms[0])
			continue;
		if (res == NULL && verbose)
			warnx("%s:%zu: ref not found: %s",
				d->fn, d->ln, d->xrs[i]);
		if (res == NULL)
			continue;

		/* Ignore duplicates. */

		if (lastres == res)
			continue;

		if (last)
			fputs(" ,\n", f);
		else
			fputs(".Sh SEE ALSO\n", f);

		fprintf(f, ".Xr %s 3", res);
		last = 1;
		lastres = res;
	}
	if (last)
		fputs("\n", f);
}
