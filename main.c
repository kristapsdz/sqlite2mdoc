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
#include <getopt.h>
#if HAVE_SANDBOX_INIT
# include <sandbox.h>
#endif
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

/* Verbose reporting. */
static int verbose;

/* Don't output any files: use stdout. */
static	int nofile;

/* Print out only filename. */
static	int filename;

static void
decl_function_add(struct parse *p, char **etext,
	size_t *etextsz, const char *cp, size_t len)
{

	if ((*etext)[*etextsz - 1] != ' ') {
		*etext = realloc(*etext, *etextsz + 2);
		if (*etext == NULL)
			err(1, NULL);
		(*etextsz)++;
		strlcat(*etext, " ", *etextsz + 1);
	}
	*etext = realloc(*etext, *etextsz + len + 1);
	if (*etext == NULL)
		err(1, NULL);
	memcpy(*etext + *etextsz, cp, len);
	*etextsz += len;
	(*etext)[*etextsz] = '\0';
}

static void
decl_function_copy(struct parse *p, char **etext,
	size_t *etextsz, const char *cp, size_t len)
{

	*etext = malloc(len + 1);
	if (*etext == NULL)
		err(1, NULL);
	memcpy(*etext, cp, len);
	*etextsz = len;
	(*etext)[*etextsz] = '\0';
}

/*
 * A C function (or variable, or whatever).
 * This is more specifically any non-preprocessor text.
 */
static int
decl_function(struct parse *p, const char *cp, size_t len)
{
	char		*ep, *lcp, *rcp;
	const char	*ncp;
	size_t		 nlen;
	struct defn	*d;
	struct decl	*e;

	/* Fetch current interface definition. */
	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/*
	 * Since C tokens are semicolon-separated, we may be invoked any
	 * number of times per a single line.
	 */
again:
	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}
	if (*cp == '\0')
		return(1);

	/* Whether we're a continuation clause. */
	if (d->multiline) {
		/* This might be NULL if we're not a continuation. */
		e = TAILQ_LAST(&d->dcqhead, declq);
		assert(DECLTYPE_C == e->type);
		assert(NULL != e);
		assert(NULL != e->text);
		assert(e->textsz);
	} else {
		assert(d->instruct == 0);
		e = calloc(1, sizeof(struct decl));
		if (e == NULL)
			err(1, NULL);
		e->type = DECLTYPE_C;
		TAILQ_INSERT_TAIL(&d->dcqhead, e, entries);
	}

	/*
	 * We begin by seeing if there's a semicolon on this line.
	 * If there is, we'll need to do some special handling.
	 */
	ep = strchr(cp, ';');
	lcp = strchr(cp, '{');
	rcp = strchr(cp, '}');

	/* We're only a partial statement (i.e., no closure). */
	if (ep == NULL && d->multiline) {
		assert(e->text != NULL);
		assert(e->textsz > 0);
		/* Is a struct starting or ending here? */
		if (d->instruct && NULL != rcp)
			d->instruct--;
		else if (NULL != lcp)
			d->instruct++;
		decl_function_add(p, &e->text, &e->textsz, cp, len);
		return(1);
	} else if (ep == NULL && !d->multiline) {
		d->multiline = 1;
		/* Is a structure starting in this line? */
		if (NULL != lcp &&
		    (rcp == NULL || rcp < lcp))
			d->instruct++;
		decl_function_copy(p, &e->text, &e->textsz, cp, len);
		return(1);
	}

	/* Position ourselves after the semicolon. */
	assert(NULL != ep);
	ncp = cp;
	nlen = (ep - cp) + 1;
	cp = ep + 1;
	len -= nlen;

	if (d->multiline) {
		assert(NULL != e->text);
		/* Don't stop the multi-line if we're in a struct. */
		if (d->instruct == 0) {
			if (lcp == NULL || lcp > cp)
				d->multiline = 0;
		} else if (NULL != rcp && rcp < cp)
			if (--d->instruct == 0)
				d->multiline = 0;
		decl_function_add(p, &e->text, &e->textsz, ncp, nlen);
	} else {
		assert(e->text == NULL);
		if (NULL != lcp && lcp < cp) {
			d->multiline = 1;
			d->instruct++;
		}
		decl_function_copy(p, &e->text, &e->textsz, ncp, nlen);
	}

	goto again;
}

/*
 * A definition is just #define followed by space followed by the name,
 * then the value of that name.
 * We ignore the latter.
 * FIXME: this does not understand multi-line CPP, but I don't think
 * there are any instances of that in sqlite3.h.
 */
static int
decl_define(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;
	size_t		 sz;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}
	if (len == 0) {
		warnx("%s:%zu: empty pre-processor "
			"constant", p->fn, p->ln);
		return(1);
	}

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/*
	 * We're parsing a preprocessor definition, but we're still
	 * waiting on a semicolon from a function definition.
	 * It might be a comment or an error.
	 */
	if (d->multiline) {
		if (verbose)
			warnx("%s:%zu: multiline declaration "
				"still open", p->fn, p->ln);
		e = TAILQ_LAST(&d->dcqhead, declq);
		assert(NULL != e);
		e->type = DECLTYPE_NEITHER;
		d->multiline = d->instruct = 0;
	}

	sz = 0;
	while (cp[sz] != '\0' && !isspace((unsigned char)cp[sz]))
		sz++;

	e = calloc(1, sizeof(struct decl));
	if (e == NULL)
		err(1, NULL);
	e->type = DECLTYPE_CPP;
	e->text = calloc(1, sz + 1);
	if (e->text == NULL)
		err(1, NULL);
	strlcpy(e->text, cp, sz + 1);
	e->textsz = sz;
	TAILQ_INSERT_TAIL(&d->dcqhead, e, entries);
	return(1);
}

/*
 * A declaration is a function, variable, preprocessor definition, or
 * really anything else until we reach a blank line.
 */
static void
decl(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;
	const char	*oldcp;
	size_t		 oldlen;

	oldcp = cp;
	oldlen = len;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/* Check closure. */
	if (*cp == '\0') {
		p->phase = PHASE_INIT;
		/* Check multiline status. */
		if (d->multiline) {
			if (verbose)
				warnx("%s:%zu: multiline declaration "
					"still open", p->fn, p->ln);
			e = TAILQ_LAST(&d->dcqhead, declq);
			assert(NULL != e);
			e->type = DECLTYPE_NEITHER;
			d->multiline = d->instruct = 0;
		}
		return;
	}

	d->fulldesc = realloc(d->fulldesc,
		d->fulldescsz + oldlen + 2);
	if (d->fulldesc == NULL)
		err(1, NULL);
	if (d->fulldescsz == 0)
		d->fulldesc[0] = '\0';
	d->fulldescsz += oldlen + 2;
	strlcat(d->fulldesc, oldcp, d->fulldescsz);
	strlcat(d->fulldesc, "\n", d->fulldescsz);
	
	/*
	 * Catch preprocessor defines, but discard all other types of
	 * preprocessor statements.
	 * We might already be in the middle of a declaration (a
	 * function declaration), but that's ok.
	 */

	if (*cp == '#') {
		len--;
		cp++;
		while (isspace((unsigned char)*cp)) {
			len--;
			cp++;
		}
		if (strncmp(cp, "define", 6) == 0)
			decl_define(p, cp + 6, len - 6);
		return;
	}

	/* Skip one-liner comments. */

	if (len > 4 &&
	    cp[0] == '/' && cp[1] == '*' &&
	    cp[len - 2] == '*' && cp[len - 1] == '/')
		return;

	decl_function(p, cp, len);
}

/*
 * Whether to end an interface description phase with an asterisk-slash.
 * This is run within a phase already opened with slash-asterisk.  It
 * adjusts the parse state on ending a phase or syntax errors.  It has
 * various hacks around lacks syntax (e.g., starting single-asterisk
 * instead of double-asterisk) found in the wild.
 *
 * Returns zero if not ending the phase, non-zero if ending.
 */
static int
endphase(struct parse *p, const char *cp)
{

	if (*cp == '\0') {
		/*
		 * Error: empty line.
		 */
		warnx("%s:%zu: warn: unexpected empty line in "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return 1;
	} else if (strcmp(cp, "*/") == 0) {
		/*
		 * End of the interface description.
		 */
		p->phase = PHASE_DECL;
		return 1;
	} else if (!(cp[0] == '*' && cp[1] == '*')) {
		/*
		 * Error: bad syntax, not end or continuation.
		 */
		if (cp[0] == '*' && cp[1] == '\0') {
			if (verbose)
				warnx("%s:%zu: warn: ignoring "
					"standalone asterisk "
					"in interface description",
					p->fn, p->ln);
			return 0;
		} else if (cp[0] == '*' && cp[1] == ' ') {
			if (verbose)
				warnx("%s:%zu: warn: ignoring "
					"leading single asterisk "
					"in interface description",
					p->fn, p->ln);
			return 0;
		}
		warnx("%s:%zu: warn: ambiguous leading characters in "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return 1;
	}

	/* If here, at a continuation ('**'). */

	return 0;
}

/*
 * Parse a "SEE ALSO" phase, which can come at any point in the
 * interface description (unlike what they claim).
 */
static void
seealso(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	/* Blank line: back to description part. */
	if (len == 0) {
		p->phase = PHASE_DESC;
		return;
	}

	/* Fetch current interface definition. */
	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	d->seealso = realloc(d->seealso,
		d->seealsosz + len + 1);
	memcpy(d->seealso + d->seealsosz, cp, len);
	d->seealsosz += len;
	d->seealso[d->seealsosz] = '\0';
}

/*
 * A definition description is a block of text that we'll later format
 * in mdoc(7).
 * It extends from the name of the definition down to the declarations
 * themselves.
 */
static void
desc(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;
	size_t		 nsz;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;

	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	/* Fetch current interface definition. */

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/* Ignore leading blank lines. */

	if (len == 0 && d->desc == NULL)
		return;

	/* Collect SEE ALSO clauses. */

	if (strncasecmp(cp, "see also:", 9) == 0) {
		cp += 9;
		len -= 9;
		while (isspace((unsigned char)*cp)) {
			cp++;
			len--;
		}
		p->phase = PHASE_SEEALSO;
		d->seealso = realloc(d->seealso,
			d->seealsosz + len + 1);
		memcpy(d->seealso + d->seealsosz, cp, len);
		d->seealsosz += len;
		d->seealso[d->seealsosz] = '\0';
		return;
	}

	/* White-space padding between lines. */

	if (d->desc != NULL &&
	    d->descsz > 0 &&
	    d->desc[d->descsz - 1] != ' ' &&
	    d->desc[d->descsz - 1] != '\n') {
		d->desc = realloc(d->desc, d->descsz + 2);
		if (d->desc == NULL)
			err(1, NULL);
		d->descsz++;
		strlcat(d->desc, " ", d->descsz + 1);
	}

	/* Either append the line of a newline, if blank. */

	nsz = len == 0 ? 1 : len;
	if (d->desc == NULL) {
		assert(d->descsz == 0);
		d->desc = calloc(1, nsz + 1);
		if (d->desc == NULL)
			err(1, NULL);
	} else {
		d->desc = realloc(d->desc, d->descsz + nsz + 1);
		if (d->desc == NULL)
			err(1, NULL);
	}

	d->descsz += nsz;
	strlcat(d->desc, len == 0 ? "\n" : cp, d->descsz + 1);
}

/*
 * Copy all KEYWORDS into a buffer.
 */
static void
keys(struct parse *p, const char *cp, size_t len)
{
	struct defn	*d;

	if (endphase(p, cp) || len < 2)
		return;

	cp += 2;
	len -= 2;
	while (isspace((unsigned char)*cp)) {
		cp++;
		len--;
	}

	if (len == 0) {
		p->phase = PHASE_DESC;
		return;
	} else if (strncmp(cp, "KEYWORDS:", 9))
		return;

	cp += 9;
	len -= 9;

	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);
	d->keybuf = realloc(d->keybuf, d->keybufsz + len + 1);
	if (d->keybuf == NULL)
		err(1, NULL);
	memcpy(d->keybuf + d->keybufsz, cp, len);
	d->keybufsz += len;
	d->keybuf[d->keybufsz] = '\0';
}

/*
 * Initial state is where we're scanning forward to find commented
 * instances of CAPI3REF.
 */
static void
init(struct parse *p, const char *cp)
{
	struct defn	*d;
	size_t		 i, sz;

	/* Look for comment hook. */

	if (cp[0] != '*' || cp[1] != '*')
		return;
	cp += 2;
	while (isspace((unsigned char)*cp))
		cp++;

	/* Look for beginning of definition. */

	if (strncmp(cp, "CAPI3REF:", 9))
		return;
	cp += 9;
	while (isspace((unsigned char)*cp))
		cp++;
	if (*cp == '\0') {
		warnx("%s:%zu: warn: unexpected end of "
			"interface definition", p->fn, p->ln);
		return;
	}

	/* Add definition to list of existing ones. */

	if ((d = calloc(1, sizeof(struct defn))) == NULL)
		err(1, NULL);
	if ((d->name = strdup(cp)) == NULL)
		err(1, NULL);

	/* Strip trailing spaces and periods. */

	for (sz = strlen(d->name); sz > 0; sz--)
		if (d->name[sz - 1] == '.' ||
		    d->name[sz - 1] == ' ')
			d->name[sz - 1] = '\0';
		else
			break;

	/*
	 * Un-title case.  Use a simple heuristic where all words
	 * starting with an upper case letter followed by a not
	 * uppercase letter are lowercased.
	 */

	for (i = 0; sz > 0 && i < sz - 1; i++)
		if ((i == 0 || d->name[i - 1] == ' ') &&
		    isupper((unsigned char)d->name[i]) &&
		    !isupper((unsigned char)d->name[i + 1]) &&
		    !ispunct((unsigned char)d->name[i + 1]))
			d->name[i] = tolower((unsigned char)d->name[i]);

	d->fn = p->fn;
	d->ln = p->ln;
	p->phase = PHASE_KEYS;
	TAILQ_INIT(&d->dcqhead);
	TAILQ_INSERT_TAIL(&p->dqhead, d, entries);
}

#define	BPOINT(_cp) \
	(';' == (_cp)[0] || \
	 '[' == (_cp)[0] || \
	 ('(' == (_cp)[0] && '*' != (_cp)[1]) || \
	 ')' == (_cp)[0] || \
	 '{' == (_cp)[0])

/*
 * Given a declaration (be it preprocessor or C), try to parse out a
 * reasonable "name" for the affair.
 * For a struct, for example, it'd be the struct name.
 * For a typedef, it'd be the type name.
 * For a function, it'd be the function name.
 */
static void
grok_name(const struct decl *e,
	const char **start, size_t *sz)
{
	const char	*cp;

	*start = NULL;
	*sz = 0;

	if (DECLTYPE_CPP != e->type) {
		if (e->text[e->textsz - 1] != ';')
			return;
		cp = e->text;
		do {
			while (isspace((unsigned char)*cp))
				cp++;
			if (BPOINT(cp))
				break;
			/* Function pointers... */
			if (*cp == '(')
				cp++;
			/* Pass over pointers. */
			while (*cp == '*')
				cp++;
			*start = cp;
			*sz = 0;
			while (!isspace((unsigned char)*cp)) {
				if (BPOINT(cp))
					break;
				cp++;
				(*sz)++;
			}
		} while (!BPOINT(cp));
	} else {
		*sz = e->textsz;
		*start = e->text;
	}
}

/*
 * Extract information from the interface definition.
 * Mark it as "postprocessed" on success.
 */
static void
postprocess(const char *prefix, struct defn *d)
{
	struct decl	*first;
	const char	*start;
	size_t		 offs, sz, i;
	ENTRY		 ent;

	if (TAILQ_EMPTY(&d->dcqhead))
		return;

	/* Find the first #define or declaration. */

	TAILQ_FOREACH(first, &d->dcqhead, entries)
		if (DECLTYPE_CPP == first->type ||
		    DECLTYPE_C == first->type)
			break;

	if (first == NULL) {
		warnx("%s:%zu: no entry to document", d->fn, d->ln);
		return;
	}

	/*
	 * Now compute the document name (`Dt').
	 * We'll also use this for the filename.
	 */

	grok_name(first, &start, &sz);
	if (start == NULL) {
		warnx("%s:%zu: couldn't deduce "
			"entry name", d->fn, d->ln);
		return;
	}

	/* Document name needs all-caps. */

	if ((d->dt = strndup(start, sz)) == NULL)
		err(1, NULL);
	sz = strlen(d->dt);
	for (i = 0; i < sz; i++)
		d->dt[i] = toupper((unsigned char)d->dt[i]);

	/* Filename needs no special chars. */

	if (filename) {
		asprintf(&d->fname, "%.*s.3", (int)sz, start);
		offs = 0;
	} else {
		asprintf(&d->fname, "%s/%.*s.3",
			prefix, (int)sz, start);
		offs = strlen(prefix) + 1;
	}

	if (d->fname == NULL)
		err(1, NULL);

	for (i = 0; i < sz; i++) {
		if (isalnum((unsigned char)d->fname[offs + i]) ||
		    d->fname[offs + i] == '_' ||
		    d->fname[offs + i] == '-')
			continue;
		d->fname[offs + i] = '_';
	}

	/*
	 * First, extract all keywords.
	 */
	for (i = 0; i < d->keybufsz; ) {
		while (isspace((unsigned char)d->keybuf[i]))
			i++;
		if (i == d->keybufsz)
			break;
		sz = 0;
		start = &d->keybuf[i];
		if (d->keybuf[i] == '{') {
			start = &d->keybuf[++i];
			for ( ; i < d->keybufsz; i++, sz++)
				if (d->keybuf[i] == '}')
					break;
			if (d->keybuf[i] == '}')
				i++;
		} else
			for ( ; i < d->keybufsz; i++, sz++)
				if (isspace((unsigned char)d->keybuf[i]))
					break;
		if (sz == 0)
			continue;
		d->keys = reallocarray(d->keys,
			d->keysz + 1, sizeof(char *));
		if (d->keys == NULL)
			err(1, NULL);
		d->keys[d->keysz] = malloc(sz + 1);
		if (d->keys[d->keysz] == NULL)
			err(1, NULL);
		memcpy(d->keys[d->keysz], start, sz);
		d->keys[d->keysz][sz] = '\0';
		d->keysz++;
		
		/* Hash the keyword. */
		ent.key = d->keys[d->keysz - 1];
		ent.data = d;
		(void)hsearch(ent, ENTER);
	}

	/*
	 * Now extract all `Nm' values for this document.
	 * We only use CPP and C references, and hope for the best when
	 * doing so.
	 * Enter each one of these as a searchable keyword.
	 */
	TAILQ_FOREACH(first, &d->dcqhead, entries) {
		if (DECLTYPE_CPP != first->type &&
		    DECLTYPE_C != first->type)
			continue;
		grok_name(first, &start, &sz);
		if (start == NULL)
			continue;
		d->nms = reallocarray(d->nms,
			d->nmsz + 1, sizeof(char *));
		if (d->nms == NULL)
			err(1, NULL);
		d->nms[d->nmsz] = malloc(sz + 1);
		if (d->nms[d->nmsz] == NULL)
			err(1, NULL);
		memcpy(d->nms[d->nmsz], start, sz);
		d->nms[d->nmsz][sz] = '\0';
		d->nmsz++;

		/* Hash the name. */
		ent.key = d->nms[d->nmsz - 1];
		ent.data = d;
		(void)hsearch(ent, ENTER);
	}

	if (d->nmsz == 0) {
		warnx("%s:%zu: couldn't deduce "
			"any names", d->fn, d->ln);
		return;
	}

	/*
	 * Next, scan for all `Xr' values.
	 * We'll add more to this list later.
	 */
	for (i = 0; i < d->seealsosz; i++) {
		/*
		 * Find next value starting with `['.
		 * There's other stuff in there (whitespace or
		 * free text leading up to these) that we're ok
		 * to ignore.
		 */
		while (i < d->seealsosz && d->seealso[i] != '[')
			i++;
		if (i == d->seealsosz)
			break;

		/*
		 * Now scan for the matching `]'.
		 * We can also have a vertical bar if we're separating a
		 * keyword and its shown name.
		 */
		start = &d->seealso[++i];
		sz = 0;
		while (i < d->seealsosz &&
		      d->seealso[i] != ']' &&
		      d->seealso[i] != '|') {
			i++;
			sz++;
		}
		if (i == d->seealsosz)
			break;
		if (sz == 0)
			continue;

		/*
		 * Continue on to the end-of-reference, if we weren't
		 * there to begin with.
		 */
		if (d->seealso[i] != ']')
			while (i < d->seealsosz &&
			      d->seealso[i] != ']')
				i++;

		/* Strip trailing whitespace. */
		while (sz > 1 && start[sz - 1] == ' ')
			sz--;

		/* Strip trailing parenthesis. */
		if (sz > 2 &&
		    start[sz - 2] == '(' &&
	 	    start[sz - 1] == ')')
			sz -= 2;

		d->xrs = reallocarray(d->xrs,
			d->xrsz + 1, sizeof(char *));
		if (d->xrs == NULL)
			err(1, NULL);
		d->xrs[d->xrsz] = malloc(sz + 1);
		if (d->xrs[d->xrsz] == NULL)
			err(1, NULL);
		memcpy(d->xrs[d->xrsz], start, sz);
		d->xrs[d->xrsz][sz] = '\0';
		d->xrsz++;
	}

	/*
	 * Next, extract all references.
	 * We'll accumulate these into a list of SEE ALSO tags, after.
	 * See how these are parsed above for a description: this is
	 * basically the same thing.
	 */
	for (i = 0; i < d->descsz; i++) {
		if (d->desc[i] != '[')
			continue;
		i++;
		if (d->desc[i] == '[')
			continue;

		start = &d->desc[i];
		for (sz = 0; i < d->descsz; i++, sz++)
			if (d->desc[i] == ']' ||
			    d->desc[i] == '|')
				break;

		if (i == d->descsz)
			break;
		else if (sz == 0)
			continue;

		if (d->desc[i] != ']')
			while (i < d->descsz && d->desc[i] != ']')
				i++;

		while (sz > 1 && start[sz - 1] == ' ')
			sz--;

		if (sz > 2 &&
		    start[sz - 2] == '(' &&
		    start[sz - 1] == ')')
			sz -= 2;

		d->xrs = reallocarray(d->xrs,
			d->xrsz + 1, sizeof(char *));
		if (d->xrs == NULL)
			err(1, NULL);
		d->xrs[d->xrsz] = malloc(sz + 1);
		if (d->xrs[d->xrsz] == NULL)
			err(1, NULL);
		memcpy(d->xrs[d->xrsz], start, sz);
		d->xrs[d->xrsz][sz] = '\0';
		d->xrsz++;
	}

	d->postprocessed = 1;
}

/*
 * Emit a valid mdoc(7) document within the given prefix.
 */
static void
print_mdoc(struct defn *d)
{
	struct decl	*first;
	size_t		 i;
	FILE		*f;

	if (!d->postprocessed) {
		warnx("%s:%zu: interface has errors, not "
			"producing manpage", d->fn, d->ln);
		return;
	}

	if (nofile == 0) {
		if ((f = fopen(d->fname, "w")) == NULL) {
			warn("%s: fopen", d->fname);
			return;
		}
	} else if (filename) {
		printf("%s\n", d->fname);
		return;
	} else
		f = stdout;

	/* Begin by outputting the mdoc(7) header. */

	fputs(".Dd $" "Mdocdate$\n", f);
	fprintf(f, ".Dt %s 3\n", d->dt);
	fputs(".Os\n", f);
	fputs(".Sh NAME\n", f);

	/* Now print the name bits of each declaration. */

	for (i = 0; i < d->nmsz; i++)
		fprintf(f, ".Nm %s%s\n", d->nms[i],
			i < d->nmsz - 1 ? " ," : "");

	fprintf(f, ".Nd %s\n", d->name);

	fputs(".Sh SYNOPSIS\n", f);
	fputs(".In sqlite3.h\n", f);

	TAILQ_FOREACH(first, &d->dcqhead, entries)
		print_synopsis(f, first, d);

	fputs(".Sh DESCRIPTION\n", f);
	print_description(f, d);

	fputs(".Sh IMPLEMENTATION NOTES\n", f);
	print_implementation(f, d, verbose);

	if (nofile == 0)
		fclose(f);
}

#if HAVE_PLEDGE
/*
 * We pledge(2) stdio if we're receiving from stdin and writing to
 * stdout, otherwise we need file-creation and writing.
 */
static void
sandbox_pledge(void)
{

	if (nofile) {
		if (pledge("stdio", NULL) == -1)
			err(1, NULL);
	} else {
		if (pledge("stdio wpath cpath", NULL) == -1)
			err(1, NULL);
	}
}
#endif

#if HAVE_SANDBOX_INIT
/*
 * Darwin's "seatbelt".
 * If we're writing to stdout, then use pure computation.
 * Otherwise we need file writing.
 */
static void
sandbox_apple(void)
{
	char	*ep;
	int	 rc;

	rc = sandbox_init
		(nofile ? kSBXProfilePureComputation :
		 kSBXProfileNoNetwork, SANDBOX_NAMED, &ep);
	if (rc == 0)
		return;
	perror(ep);
	sandbox_free_error(ep);
	exit(1);
}
#endif

/*
 * Check to see whether there are any filename duplicates.
 * This is just a warning, but will really screw things up, since the
 * last filename will overwrite the first.
 */
static void
check_dupes(struct parse *p)
{
	const struct defn	*d, *dd;

	TAILQ_FOREACH(d, &p->dqhead, entries)
		TAILQ_FOREACH_REVERSE(dd, &p->dqhead, defnq, entries) {
			if (dd == d)
				break;
			if (d->fname == NULL ||
			    dd->fname == NULL ||
			    strcmp(d->fname, dd->fname))
				continue;
			warnx("%s:%zu: duplicate filename: "
				"%s (from %s, line %zu)", d->fn,
				d->ln, d->fname, dd->nms[0], dd->ln);
		}
}

int
main(int argc, char *argv[])
{
	size_t		 i, bufsz;
	ssize_t		 len;
	FILE		*f = stdin;
	char		*cp = NULL;
	const char	*prefix = ".";
	struct parse	 p;
	int		 rc = 0, ch;
	struct defn	*d;
	struct decl	*e;

	memset(&p, 0, sizeof(struct parse));

	p.fn = "<stdin>";
	p.ln = 0;
	p.phase = PHASE_INIT;

	TAILQ_INIT(&p.dqhead);

	while ((ch = getopt(argc, argv, "nNp:v")) != -1)
		switch (ch) {
		case 'n':
			nofile = 1;
			break;
		case 'N':
			nofile = 1;
			filename = 1;
			break;
		case 'p':
			prefix = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	if (argc > 1)
		goto usage;

	if (argc > 0) {
		if ((f = fopen(argv[0], "r")) == NULL)
			err(1, "%s", argv[0]);
		p.fn = argv[0];
	}

#if HAVE_SANDBOX_INIT
	sandbox_apple();
#elif HAVE_PLEDGE
	sandbox_pledge();
#endif
	/*
	 * Read in line-by-line and process in the phase dictated by our
	 * finite state automaton.
	 */
	
	while ((len = getline(&cp, &bufsz, f)) != -1) {
		assert(len > 0);
		p.ln++;
		if (cp[len - 1] != '\n') {
			warnx("%s:%zu: unterminated line", p.fn, p.ln);
			break;
		}

		/*
		 * Lines are now always NUL-terminated, and don't allow
		 * NUL characters in the line.
		 */

		cp[--len] = '\0';
		len = strlen(cp);

		switch (p.phase) {
		case PHASE_INIT:
			init(&p, cp);
			break;
		case PHASE_KEYS:
			keys(&p, cp, (size_t)len);
			break;
		case PHASE_DESC:
			desc(&p, cp, (size_t)len);
			break;
		case PHASE_SEEALSO:
			seealso(&p, cp, (size_t)len);
			break;
		case PHASE_DECL:
			decl(&p, cp, (size_t)len);
			break;
		}
	}

	/*
	 * If we hit the last line, then try to process.
	 * Otherwise, we failed along the way.
	 */

	if (feof(f)) {
		/*
		 * Allow us to be at the declarations or scanning for
		 * the next clause.
		 */
		if (p.phase == PHASE_INIT ||
		    p.phase == PHASE_DECL) {
			if (hcreate(5000) == 0)
				err(1, NULL);
			TAILQ_FOREACH(d, &p.dqhead, entries)
				postprocess(prefix, d);
			check_dupes(&p);
			TAILQ_FOREACH(d, &p.dqhead, entries)
				print_mdoc(d);
			rc = 1;
		} else if (p.phase != PHASE_DECL)
			warnx("%s:%zu: exit when not in "
				"initial state", p.fn, p.ln);
	}

	while ((d = TAILQ_FIRST(&p.dqhead)) != NULL) {
		TAILQ_REMOVE(&p.dqhead, d, entries);
		while ((e = TAILQ_FIRST(&d->dcqhead)) != NULL) {
			TAILQ_REMOVE(&d->dcqhead, e, entries);
			free(e->text);
			free(e);
		}
		free(d->name);
		free(d->desc);
		free(d->fulldesc);
		free(d->dt);
		for (i = 0; i < d->nmsz; i++)
			free(d->nms[i]);
		for (i = 0; i < d->xrsz; i++)
			free(d->xrs[i]);
		for (i = 0; i < d->keysz; i++)
			free(d->keys[i]);
		free(d->keys);
		free(d->nms);
		free(d->xrs);
		free(d->fname);
		free(d->seealso);
		free(d->keybuf);
		free(d);
	}

	return !rc;
usage:
	fprintf(stderr, "usage: %s [-Nnv] [-p prefix] [file]\n",
		getprogname());
	return 1;
}
