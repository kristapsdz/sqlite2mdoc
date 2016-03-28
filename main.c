/*	$Id$ */
/*
 * Copyright (c) 2016 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <sys/queue.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Phase of parsing input file.
 */
enum	phase {
	PHASE_INIT = 0, /* waiting to encounter definition */
	PHASE_KEYS, /* have definition, now keywords */
	PHASE_DESC, /* have keywords, now description */
	PHASE_DECL /* have description, now declarations */
};

/*
 * What kind of declaration (preliminary analysis). 
 */
enum	decltype {
	DECLTYPE_CPP, /* pre-processor */
	DECLTYPE_C, /* semicolon-closed non-preprocessor */
	DECLTYPE_NEITHER /* non-preprocessor, no semicolon */
};

enum	preproc {
	PREPROC_SQLITE_API,
	PREPROC_SQLITE_DEPRECATED,
	PREPROC_SQLITE_EXPERIMENTAL,
	PREPROC_SQLITE_EXTERN,
	PREPROC__MAX
};

enum	tag {
	TAG_BLOCK_CLOSE,
	TAG_BLOCK_OPEN,
	TAG_DD_CLOSE,
	TAG_DD_OPEN,
	TAG_DL_CLOSE,
	TAG_DL_OPEN,
	TAG_DT_CLOSE,
	TAG_DT_OPEN,
	TAG_LI_CLOSE,
	TAG_LI_OPEN,
	TAG_OL_CLOSE,
	TAG_OL_OPEN,
	TAG_PRE_CLOSE,
	TAG_PRE_OPEN,
	TAG_UL_CLOSE,
	TAG_UL_OPEN,
	TAG__MAX
};

TAILQ_HEAD(defnq, defn);
TAILQ_HEAD(declq, decl);

/*
 * A declaration of type DECLTYPE_CPP or DECLTYPE_C.
 * These need not be unique (if ifdef'd).
 */
struct	decl {
	enum decltype	 type; /* type of declaration */
	char		*text; /* text */
	size_t		 textsz; /* strlen(text) */
	TAILQ_ENTRY(decl) entries;
};

/*
 * A definition is basically the manpage contents.
 */
struct	defn {
	char		*name; /* really Nd */
	TAILQ_ENTRY(defn) entries;
	char		*desc; /* long description */
	size_t		 descsz; /* strlen(desc) */
	struct declq	 dcqhead; /* declarations */
	int		 multiline; /* used when parsing */
	int		 instruct; /* used when parsing */
	const char	*fn; /* parsed from file */
	size_t		 ln; /* parsed at line */
};

/*
 * Entire parse routine.
 */
struct	parse {
	enum phase	 phase; /* phase of parse */
	size_t		 ln; /* line number */
	const char	*fn; /* open file */
	struct defnq	 dqhead; /* definitions */
};

struct	taginfo {
	const char	*html;
	const char	*mdoc;
	unsigned int	 flags;
#define	TAGINFO_NOBR	 0x01
#define	TAGINFO_NOOP	 0x02
#define	TAGINFO_NOSP	 0x04
#define	TAGINFO_INLINE	 0x08
};

static	const struct taginfo tags[TAG__MAX] = {
	{ "</blockquote>", ".Ed\n.Pp", 0 }, /* TAG_BLOCK_CLOSE */
	{ "<blockquote>", ".Bd -ragged -offset Ds", 0 }, /* TAG_BLOCK_OPEN */
	{ "</dd>", "", TAGINFO_NOOP }, /* TAG_DD_CLOSE */
	{ "<dd>", "", TAGINFO_NOOP }, /* TAG_DD_OPEN */
	{ "</dl>", ".El\n.Pp", 0 }, /* TAG_DL_CLOSE */
	{ "<dl>", ".Bl -tag -width Ds", 0 }, /* TAG_DL_OPEN */
	{ "</dt>", "", TAGINFO_NOBR | TAGINFO_NOSP}, /* TAG_DT_CLOSE */
	{ "<dt>", ".It", TAGINFO_NOBR }, /* TAG_DT_OPEN */
	{ "</li>", "", TAGINFO_NOOP }, /* TAG_LI_CLOSE */
	{ "<li>", ".It", 0 }, /* TAG_LI_OPEN */
	{ "</ol>", ".El\n.Pp", 0 }, /* TAG_OL_CLOSE */
	{ "<ol>", ".Bl -enum", 0 }, /* TAG_OL_OPEN */
	{ "</pre>", ".Ed\n.Pp", 0 }, /* TAG_PRE_CLOSE */
	{ "<pre>", ".Bd -literal -offset Ds", 0 }, /* TAG_PRE_OPEN */
	{ "</ul>", ".El\n.Pp", 0 }, /* TAG_UL_CLOSE */
	{ "<ul>", ".Bl -bullet", 0 }, /* TAG_UL_OPEN */
};

static	const char *const preprocs[TAG__MAX] = {
	"SQLITE_API", /* PREPROC_SQLITE_API */
	"SQLITE_DEPRECATED", /* PREPROC_SQLITE_DEPRECATED */
	"SQLITE_EXPERIMENTAL", /* PREPROC_SQLITE_EXPERIMENTAL */
	"SQLITE_EXTERN", /* PREPROC_SQLITE_EXTERN */
};

static void
decl_function_add(struct parse *p, char **etext, 
	size_t *etextsz, const char *cp, size_t len)
{

	if (' ' != (*etext)[*etextsz - 1]) {
		*etext = realloc(*etext, *etextsz + 2);
		if (NULL == *etext)
			err(EXIT_FAILURE, "%s:%zu: realloc", p->fn, p->ln);
		(*etextsz)++;
		strlcat(*etext, " ", *etextsz + 1);
	}
	*etext = realloc(*etext, *etextsz + len + 1);
	if (NULL == *etext)
		err(EXIT_FAILURE, "%s:%zu: realloc", p->fn, p->ln);
	memcpy(*etext + *etextsz, cp, len);
	*etextsz += len;
	(*etext)[*etextsz] = '\0';
}

static void
decl_function_copy(struct parse *p, char **etext,
	size_t *etextsz, const char *cp, size_t len)
{

	*etext = malloc(len + 1);
	if (NULL == *etext)
		err(EXIT_FAILURE, "%s:%zu: strdup", p->fn, p->ln);
	memcpy(*etext, cp, len);
	*etextsz = len;
	(*etext)[*etextsz] = '\0';
}

/*
 * A C function (or variable, or whatever).
 * This is more specifically any non-preprocessor text.
 */
static int
decl_function(struct parse *p, char *cp, size_t len)
{
	char		*ep, *ncp, *lcp, *rcp;
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
	while (isspace((int)*cp)) {
		cp++;
		len--;
	}
	if ('\0' == *cp)
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
		assert(0 == d->instruct);
		e = calloc(1, sizeof(struct decl));
		e->type = DECLTYPE_C;
		if (NULL == e)
			err(EXIT_FAILURE, "%s:%zu: calloc", p->fn, p->ln);
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
	if (NULL == ep && d->multiline) {
		assert(NULL != e->text);
		assert(e->textsz > 0);
		/* Is a struct starting or ending here? */
		if (d->instruct && NULL != rcp)
			d->instruct--;
		else if (NULL != lcp)
			d->instruct++;
		decl_function_add(p, &e->text, &e->textsz, cp, len);
		return(1);
	} else if (NULL == ep && ! d->multiline) {
		d->multiline = 1;
		/* Is a structure starting in this line? */
		if (NULL != lcp && 
		    (NULL == rcp || rcp < lcp))
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
		if (0 == d->instruct) {
			if (NULL == lcp || lcp > cp)
				d->multiline = 0;
		} else if (NULL != rcp && rcp < cp)
			if (0 == --d->instruct)
				d->multiline = 0;
		decl_function_add(p, &e->text, &e->textsz, ncp, nlen);
	} else {
		assert(NULL == e->text);
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
 * FIXME: this does not understand multi-line CPP.
 */
static int
decl_define(struct parse *p, char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;
	size_t		 sz;

	while (isspace((int)*cp)) {
		cp++;
		len--;
	}
	if (0 == len) {
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
		warnx("%s:%zu: multiline declaration "
			"still open", p->fn, p->ln);
		e = TAILQ_LAST(&d->dcqhead, declq);
		assert(NULL != e);
		e->type = DECLTYPE_NEITHER;
		d->multiline = d->instruct = 0;
	}

	sz = 0;
	while ('\0' != cp[sz] && ! isspace((int)cp[sz]))
		sz++;

	e = calloc(1, sizeof(struct decl));
	if (NULL == e) 
		err(EXIT_FAILURE, "%s:%zu: calloc", p->fn, p->ln);
	e->type = DECLTYPE_CPP;
	e->text = calloc(1, sz + 1);
	if (NULL == e->text)
		err(EXIT_FAILURE, "%s:%zu: calloc", p->fn, p->ln);
	strlcpy(e->text, cp, sz + 1);
	e->textsz = sz;
	TAILQ_INSERT_TAIL(&d->dcqhead, e, entries);
	return(1);
}

/*
 * A declaration is a function, variable, preprocessor definition, or
 * really anything else until we reach a blank line.
 */
static int
decl(struct parse *p, char *cp, size_t len)
{
	struct defn	*d;
	struct decl	*e;

	while (isspace((int)*cp)) {
		cp++;
		len--;
	}

	/* Check closure. */
	if ('\0' == *cp) {
		p->phase = PHASE_INIT;
		/* Check multiline status. */
		d = TAILQ_LAST(&p->dqhead, defnq);
		assert(NULL != d);
		if (d->multiline) {
			warnx("%s:%zu: multiline declaration "
				"still open", p->fn, p->ln);
			e = TAILQ_LAST(&d->dcqhead, declq);
			assert(NULL != e);
			e->type = DECLTYPE_NEITHER;
			d->multiline = d->instruct = 0;
		}
		return(1);
	} 
	
	/* 
	 * Catch preprocessor defines, but discard all other types of
	 * preprocessor statements.
	 */
	if (0 == strncmp(cp, "#define", 7))
		return(decl_define(p, cp + 7, len - 7));
	else if ('#' == *cp)
		return(1);

	return(decl_function(p, cp, len));
}

/*
 * A definition description is a block of text that we'll later format
 * in mdoc(7).
 * It extends from the name of the definition down to the declarations
 * themselves.
 */
static int
desc(struct parse *p, char *cp, size_t len)
{
	struct defn	*d;
	size_t		 nsz;

	if ('\0' == *cp) {
		warnx("%s:%zu: warn: unexpected end of "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return(1);
	} else if (0 == strcmp(cp, "*/")) {
		/* End of comment area, start of declarations. */
		p->phase = PHASE_DECL;
		return(1);
	} else if ('*' != cp[0] || '*' != cp[1]) {
		warnx("%s:%zu: warn: unexpected end of "
			"interface description", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return(1);
	}
	cp += 2;
	len -= 2;
	while (isspace((int)*cp)) {
		cp++;
		len--;
	}

	/* Fetch current interface definition. */
	d = TAILQ_LAST(&p->dqhead, defnq);
	assert(NULL != d);

	/* Ignore leading blank lines. */
	if (0 == len && NULL == d->desc)
		return(1);

	/* White-space padding between lines. */
	if (NULL != d->desc && 
	    ' ' != d->desc[d->descsz - 1] &&
	    '\n' != d->desc[d->descsz - 1]) {
		d->desc = realloc(d->desc, d->descsz + 2);
		if (NULL == d->desc)
			err(EXIT_FAILURE, "%s:%zu: realloc", p->fn, p->ln);
		d->descsz++;
		strlcat(d->desc, " ", d->descsz + 1);
	}

	/* Either append the line of a newline, if blank. */
	nsz = 0 == len ? 1 : len;
	if (NULL == d->desc) {
		d->desc = calloc(1, nsz + 1);
		if (NULL == d->desc)
			err(EXIT_FAILURE, "%s:%zu: calloc", p->fn, p->ln);
	} else {
		d->desc = realloc(d->desc, d->descsz + nsz + 1);
		if (NULL == d->desc)
			err(EXIT_FAILURE, "%s:%zu: realloc", p->fn, p->ln);
	}
	d->descsz += nsz;
	strlcat(d->desc, 0 == len ? "\n" : cp, d->descsz + 1);
	return(1);
}

static int
keys(struct parse *p, char *cp, size_t len)
{

	if ('\0' == *cp) {
		warnx("%s:%zu: warn: unexpected end of "
			"interface keywords", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return(1);
	} else if ('*' != cp[0] || '*' != cp[1]) {
		warnx("%s:%zu: warn: unexpected end of "
			"interface keywords", p->fn, p->ln);
		p->phase = PHASE_INIT;
		return(1);
	}
	cp += 2;
	len -= 2;
	while (isspace((int)*cp)) {
		cp++;
		len--;
	}
	if (0 == len) {
		p->phase = PHASE_DESC;
		return(1);
	}

	return(1);
}

/*
 * Initial state is where we're scanning forward to find commented
 * instances of CAPI3REF.
 */
static int
init(struct parse *p, char *cp)
{
	struct defn	*d;

	/* Look for comment hook. */
	if ('*' != cp[0] || '*' != cp[1])
		return(1);
	cp += 2;
	while (isspace((int)*cp))
		cp++;

	/* Look for beginning of definition. */
	if (strncmp(cp, "CAPI3REF:", 9))
		return(1);
	cp += 9;
	while (isspace((int)*cp))
		cp++;
	if ('\0' == *cp) {
		warnx("%s:%zu: warn: unexpected end of "
			"interface definition", p->fn, p->ln);
		return(1);
	}

	/* Add definition to list of existing ones. */
	d = calloc(1, sizeof(struct defn));
	if (NULL == d)
		err(EXIT_FAILURE, "%s:%zu: calloc", p->fn, p->ln);
	d->name = strdup(cp);
	if (NULL == d->name)
		err(EXIT_FAILURE, "%s:%zu: strdup", p->fn, p->ln);
	d->fn = p->fn;
	d->ln = p->ln;
	p->phase = PHASE_KEYS;
	TAILQ_INIT(&d->dcqhead);
	TAILQ_INSERT_TAIL(&p->dqhead, d, entries);
	return(1);
}

#define	BPOINT(_cp) \
	(';' == (_cp) || '[' == (_cp) || \
	 '(' == (_cp) || '{' == (_cp))

static void
emit_grok_name(const struct decl *e, 
	const char **start, size_t *sz)
{
	const char	*cp;

	*start = NULL;
	*sz = 0;

	if (DECLTYPE_CPP != e->type) {
		assert(';' == e->text[e->textsz - 1]);
		cp = e->text;
		do {
			while (isspace((int)*cp))
				cp++;
			if (BPOINT(*cp))
				break;
			while ('*' == *cp)
				cp++;
			*start = cp;
			*sz = 0;
			while ( ! isspace((int)*cp)) {
				if (BPOINT(*cp))
					break;
				cp++;
				(*sz)++;
			}
		} while ( ! BPOINT(*cp));
	} else {
		*sz = e->textsz;
		*start = e->text;
	}
}

/*
 * Emit a valid mdoc(7) document within the given prefix.
 * FIXME: escaping non-mdoc(7) characters.
 */
static void
emit(const char *prefix, const struct defn *d)
{
	struct decl	*first;
	const char	*start;
	size_t		 offs, sz, i, col;
	int		 last;
	FILE		*f;
	char		*path, *cp;
	enum tag	 tag;
	enum preproc	 pre;

	if (TAILQ_EMPTY(&d->dcqhead))
		return;

	/* Find the first #define or declaration. */
	TAILQ_FOREACH(first, &d->dcqhead, entries)
		if (DECLTYPE_CPP == first->type ||
		    DECLTYPE_C == first->type)
			break;

	if (NULL == first) {
		warnx("%s:%zu: no entry to document", d->fn, d->ln);
		return;
	}

	emit_grok_name(first, &start, &sz);
	if (NULL == start) {
		warnx("%s:%zu: couldn't deduce "
			"entry name", d->fn, d->ln);
		return;
	}

	/*
	 * Open the output file.
	 * We put this in the given directory and force the filename to
	 * be only alphanumerics or some select other characters.
	 */
	asprintf(&path, "%s/%.*s.3", 
		prefix, (int)sz, start);
	if (NULL == path)
		err(EXIT_FAILURE, "asprintf");
	offs = strlen(prefix) + 1;
	for (i = 0; i < sz; i++) {
		if (isalnum((int)path[offs + i]) ||
		    '_' == path[offs + i] ||
		    '-' == path[offs + i])
			continue;
		path[offs + i] = '_';
	}
	if (NULL == (f = fopen(path, "w"))) {
		warn("%s: fopen", path);
		free(path);
		return;
	}
	free(path);

	/* 
	 * Begin by outputting the mdoc(7) header. 
	 * We use the first real bits as the title of the page, which
	 * are also used for the file-name.
	 */
	fputs(".Dd $Mdocdate$\n", f);
	fputs(".Dt ", f);
	for (i = 0; i < sz; i++)
		fputc(toupper((int)start[i]), f);
	fputs(" 3\n", f);
	fputs(".Os\n", f);
	fputs(".Sh NAME\n", f);

	/* 
	 * Now print the name bits of each declaration. 
	 * We need some extra logic to handle the comma that follows
	 * multiple names.
	 */
	last = 0;
	TAILQ_FOREACH(first, &d->dcqhead, entries) {
		if (DECLTYPE_CPP != first->type &&
		    DECLTYPE_C != first->type)
			continue;
		emit_grok_name(first, &start, &sz);
		if (NULL == start) 
			continue;
		if (last) 
			fputs(" ,\n", f);
		fprintf(f, ".Nm %.*s", (int)sz, start);
		last = 1;
	}

	if (last) 
		fputs("\n", f);
	fprintf(f, ".Nd %s\n", d->name);

	fputs(".Sh SYNOPSIS\n", f);
	TAILQ_FOREACH(first, &d->dcqhead, entries) {
		if (DECLTYPE_CPP != first->type &&
		    DECLTYPE_C != first->type)
			continue;

		/* Easy: just print the CPP name. */
		if (DECLTYPE_CPP == first->type) {
			fprintf(f, ".Fd #define %s\n",
				first->text);
			continue;
		}

		/* First, strip out the sqlite CPPs. */
		for (i = 0; i < first->textsz; i++) {
			for (pre = 0; pre < PREPROC__MAX; pre++) {
				sz = strlen(preprocs[pre]);
				if (strncmp(preprocs[pre], 
				    &first->text[i], sz))
					continue;
				i += sz;
				while (isspace((int)first->text[i]))
					i++;
				i--;
				break;
			}
			if (pre == PREPROC__MAX)
				break;
		}

		/* If we're a typedef, immediately print Vt. */
		if (0 == strncmp(&first->text[i], "typedef", 7)) {
			fprintf(f, ".Vt %s\n", &first->text[i]);
			continue;
		}

		/* Are we a struct? */
		if (first->textsz > 2 && 
		    '}' == first->text[first->textsz - 2] &&
		    NULL != (cp = strchr(&first->text[i], '{'))) {
			*cp = '\0';
			fprintf(f, ".Vt %s;\n", &first->text[i]);
			/* Restore brace for later usage. */
			*cp = '{';
			continue;
		}

		/* Catch remaining non-functions. */
		if (first->textsz > 2 &&
		    ')' != first->text[first->textsz - 2]) {
			fprintf(f, ".Vt %s\n", &first->text[i]);
			continue;
		}

		fputs(".Bd -literal\n", f);
		fputs(&first->text[i], f);
		fputs("\n.Ed\n", f);
	}

	fputs(".Sh DESCRIPTION\n", f);

	/* 
	 * Strip the crap out of the description.
	 * "Crap" consists of things I don't understand that mess up
	 * parsing of the HTML, for instance,
	 *   <dl>[[foo bar]]<dt>foo bar</dt>...</dl>
	 * These are not well-formed HTML.
	 */
	for (i = 0; i < d->descsz; i++) {
		if ('^' == d->desc[i] && 
		    '(' == d->desc[i + 1]) {
			d->desc[i] = d->desc[i + 1] = ' ';
			i++;
			continue;
		} else if (')' == d->desc[i] && 
			   '^' == d->desc[i + 1]) {
			d->desc[i] = d->desc[i + 1] = ' ';
			i++;
			continue;
		} else if ('^' == d->desc[i]) {
			d->desc[i] = ' ';
			continue;
		} else if ('[' != d->desc[i] || 
			   '[' != d->desc[i + 1]) 
			continue;
		d->desc[i] = d->desc[i + 1] = ' ';
		for (i += 2; i < d->descsz; i++) {
			if (']' == d->desc[i] && 
			    ']' == d->desc[i + 1]) 
				break;
			d->desc[i] = ' ';
		}
		if (i == d->descsz)
			continue;
		d->desc[i] = d->desc[i + 1] = ' ';
		i++;
	}

#if 0
	fputs(d->desc, f);
	fputs("\n", f);
#endif

	/*
	 * Here we go!
	 * Print out the description as best we can.
	 * Do on-the-fly processing of any HTML we encounter into
	 * mdoc(7) and try to break lines up.
	 */
	col = 0;
	for (i = 0; i < d->descsz; ) {
		/* 
		 * Newlines are paragraph breaks.
		 * If we have multiple newlines, then keep to a single
		 * `Pp' to keep it clean.
		 * Only do this if we're not before a block-level HTML,
		 * as this would mean, for instance, a `Pp'-`Bd' pair.
		 */
		if ('\n' == d->desc[i]) {
			while (isspace((int)d->desc[i]))
				i++;
			for (tag = 0; tag < TAG__MAX; tag++) {
				sz = strlen(tags[tag].html);
				if (0 == strncmp(&d->desc[i], tags[tag].html, sz))
					break;
			}
			if (TAG__MAX == tag ||
			    TAGINFO_INLINE & tags[tag].flags) {
				if (col > 0)
					fputs("\n", f);
				fputs(".Pp\n", f);
				/* We're on a new line. */
				col = 0;
			}
			continue;
		}

		/*
		 * New sentence, new line.
		 * We guess whether this is the case by using the
		 * dumbest possible heuristic.
		 */
		if (' ' == d->desc[i] && i &&
		    '.' == d->desc[i - 1]) {
			while (' ' == d->desc[i])
				i++;
			fputs("\n", f);
			col = 0;
			continue;
		}
		/*
		 * After 65 characters, force a break when we encounter
		 * white-space to keep our lines more or less tidy.
		 */
		if (col > 65 && ' ' == d->desc[i]) {
			while (' ' == d->desc[i]) 
				i++;
			fputs("\n", f);
			col = 0;
			continue;
		}

		/*
		 * Parsing HTML tags.
		 * Why, sqlite guys, couldn't you have used something
		 * like markdown or something?  
		 * Sheesh.
		 */
		if ('<' == d->desc[i]) {
			for (tag = 0; tag < TAG__MAX; tag++) {
				sz = strlen(tags[tag].html);
				if (strncmp(&d->desc[i], tags[tag].html, sz))
					continue;
				/*
				 * NOOP tags don't do anything, such as
				 * the case of `</dd>', which only
				 * serves to end an `It' block that will
				 * be closed out by a subsequent `It' or
				 * end of clause `El' anyway.
				 * Skip the trailing space.
				 */
				if (TAGINFO_NOOP & tags[tag].flags) {
					i += sz;
					while (isspace((int)d->desc[i]))
						i++;
					break;
				}

				/* 
				 * A breaking mdoc(7) statement.
				 * Break the current line, output the
				 * macro, and conditionally break
				 * following that (or we might do
				 * nothing at all).
				 */
				if (col > 0) {
					fputs("\n", f);
					col = 0;
				}
				fputs(tags[tag].mdoc, f);
				if ( ! (TAGINFO_NOBR & tags[tag].flags)) {
					fputs("\n", f);
					col = 0;
				} else if ( ! (TAGINFO_NOSP & tags[tag].flags)) {
					fputs(" ", f);
					col++;
				}
				i += sz;
				while (isspace((int)d->desc[i]))
					i++;
				break;
			}
			if (tag < TAG__MAX)
				continue;
		}

		if (' ' == d->desc[i] && 0 == col) {
			while (' ' == d->desc[i])
				i++;
			continue;
		}


		assert('\n' != d->desc[i]);
		fputc(d->desc[i], f);
		col++;
		i++;
	}

	fputs("\n", f);
	fclose(f);
}

int
main(int argc, char *argv[])
{
	size_t		 len;
	FILE		*f;
	char		*cp;
	const char	*prefix;
	struct parse	 p;
	int		 rc, ch;
	struct defn	*d;
	struct decl	*e;

	rc = 0;
	prefix = ".";
	f = stdin;
	memset(&p, 0, sizeof(struct parse));
	p.fn = "<stdin>";
	p.ln = 0;
	p.phase = PHASE_INIT;
	TAILQ_INIT(&p.dqhead);

	while (-1 != (ch = getopt(argc, argv, "p:")))
		switch (ch) {
		case ('p'):
			prefix = optarg;
			break;
		default:
			goto usage;
		}

	/*
	 * Read in line-by-line and process in the phase dictated by our
	 * finite state automaton.
	 */
	while (NULL != (cp = fgetln(f, &len))) {
		assert(len > 0);
		p.ln++;
		if ('\n' != cp[len - 1]) {
			warnx("%s:%zu: unterminated line", p.fn, p.ln);
			break;
		}
		cp[--len] = '\0';
		/* Lines are always nil-terminated. */
		switch (p.phase) {
		case (PHASE_INIT):
			if (init(&p, cp))
				continue;
			break;
		case (PHASE_KEYS):
			if (keys(&p, cp, len))
				continue;
			break;
		case (PHASE_DESC):
			if (desc(&p, cp, len))
				continue;
			break;
		case (PHASE_DECL):
			if (decl(&p, cp, len))
				continue;
			break;
		}
		break;
	}

	if (NULL == cp) {
		if (PHASE_INIT == p.phase) {
			TAILQ_FOREACH(d, &p.dqhead, entries)
				emit(prefix, d);
			rc = 1;
		} else
			warnx("%s:%zu: exit when not in "
				"initial state", p.fn, p.ln);
	}

	while ( ! TAILQ_EMPTY(&p.dqhead)) {
		d = TAILQ_FIRST(&p.dqhead);
		TAILQ_REMOVE(&p.dqhead, d, entries);
		while ( ! TAILQ_EMPTY(&d->dcqhead)) {
			e = TAILQ_FIRST(&d->dcqhead);
			TAILQ_REMOVE(&d->dcqhead, e, entries);
			free(e->text);
			free(e);
		}
		free(d->name);
		free(d->desc);
		free(d);
	}

	return(rc ? EXIT_SUCCESS : EXIT_FAILURE);
usage:
	fprintf(stderr, "usage: %s [-p prefix]\n", getprogname());
	return(EXIT_FAILURE);
}
