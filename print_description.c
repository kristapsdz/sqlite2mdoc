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

enum tag
{
	TAG_A,
	TAG_B,
	TAG_BLOCK,
	TAG_BR,
	TAG_DD,
	TAG_DL,
	TAG_DT,
	TAG_EM,
	TAG_H3,
	TAG_I,
	TAG_LI,
	TAG_OL,
	TAG_P,
	TAG_PRE,
	TAG_SPAN,
	TAG_TABLE,
	TAG_TD,
	TAG_TH,
	TAG_TR,
	TAG_U,
	TAG_UL,
	TAG__MAX,
};

enum attr
{
	ATTR_HREF,
	ATTR__MAX,
};

/*
 * How to handle mdoc(7) replacement content for HTML found in the text.
 */
struct taginfo
{
	const char	*omdoc; /* opening mdoc(7) */
	const char	*cmdoc; /* closing mdoc(7) */
	unsigned int	 oflags; /* opening flags */
	unsigned int	 cflags; /* closing flags */
#define	TAGINFO_NONE	 0    /* follow w/newline (default) */
#define	TAGINFO_NOBR	 0x01 /* follow w/space, not newline */
#define	TAGINFO_NOOP	 0x02 /* just strip out */
#define	TAGINFO_NOSP	 0x04 /* follow w/o space or newline */
#define	TAGINFO_INLINE	 0x08 /* inline block */
};

static const struct taginfo tags[TAG__MAX] = {
	{ "", "", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_A */
	{ "\\fB", "\\fP", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_B */
	{ ".Bd -ragged", ".Ed\n.Pp", 0, 0 }, /* TAG_BLOCK */
	{ " ", "", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_BR */
	{ "", "", TAGINFO_NOBR|TAGINFO_NOSP, TAGINFO_NOOP }, /* TAG_DD */
	{ ".Bl -tag -width Ds", ".El\n.Pp", 0, 0 }, /* TAG_DL */
	{ ".It", "", TAGINFO_NOBR, TAGINFO_NOBR|TAGINFO_NOSP }, /* TAG_DT */
	{ "\\fB", "\\fP", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_EM */
	{ ".Ss", "", TAGINFO_NOBR, TAGINFO_NOBR|TAGINFO_NOSP }, /* TAG_H3 */
	{ "\\fI", "\\fP", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_I */
	{ ".It", "", 0, TAGINFO_NOOP }, /* TAG_LI */
	{ ".Bl -enum", ".El\n.Pp", 0, 0 }, /* TAG_OL */
	{ ".Pp", "", 0, 0 }, /* TAG_P */
	{ ".Bd -literal", ".Ed\n.Pp", 0, 0 }, /* TAG_PRE */
	{ "", "", TAGINFO_INLINE, TAGINFO_INLINE}, /* TAG_SPAN */
	{ ".TS", ".TE", 0, 0 }, /* TAG_TABLE */
	{ "", "", TAGINFO_NOOP, TAGINFO_NOOP }, /* TAG_TD */
	{ "", "", TAGINFO_NOOP, TAGINFO_NOOP }, /* TAG_TH */
	{ "", "", TAGINFO_NOOP, TAGINFO_NOOP }, /* TAG_TR */
	{ "\\fI", "\\fP", TAGINFO_INLINE, TAGINFO_INLINE }, /* TAG_U */
	{ ".Bl -bullet", ".El\n.Pp", 0, 0 }, /* TAG_UL */
};

static	const char *tagnames[TAG__MAX] = {
	"a", /* TAG_A */
	"b", /* TAG_B */
	"blockquote", /* TAG_BLOCK */
	"br", /* TAG_BR */
	"dd", /* TAG_DD */
	"dl", /* TAG_DL */
	"dt", /* TAG_DT */
	"em", /* TAG_EM */
	"h3", /* TAG_H3 */
	"i", /* TAG_I */
	"li", /* TAG_LI */
	"ol", /* TAG_OL */
	"p", /* TAG_P */
	"pre", /* TAG_PRE */
	"span", /* TAG_SPAN */
	"table", /* TAG_TABLE */
	"td", /* TAG_TD */
	"th", /* TAG_TH */
	"tr", /* TAG_TR */
	"u", /* TAG_U */
	"ul", /* TAG_UL */
};

static	const char *attrs[ATTR__MAX] = {
	"href", /* ATTR_HREF */
};

static enum tag
parse_tags(const char *in, size_t *outpos, const char **outattrs,
    size_t *outattrsz, int *close)
{
	enum tag	 tag;
	enum attr	 attr;
	size_t		 sz;
	const char	*start = in;

	if (outpos != NULL)
		*outpos = 0;
	for (attr = 0; attr < ATTR__MAX; attr++) {
		if (outattrs != NULL)
			outattrs[attr] = NULL;
		if (outattrsz != NULL)
			outattrsz[attr] = 0;
	}

	/* Only scan if starting with the tag delimiter. */

	if (*in++ != '<')
		return TAG__MAX;

	if (*in == '/') {
		if (close != NULL)
			*close = 1;
		in++;
	} else if (close != NULL)
		*close = 0;

	/*
	 * Find the tag, which must be normatively formatted as either
	 * "<tag " or "<tag>".  Sets "tag", if found; otherwise, "tag"
	 * will be set to TAG__MAX on exiting the loop.
	 */

	for (tag = 0; tag < TAG__MAX; tag++) {
		sz = strlen(tagnames[tag]);
		assert(sz > 0);
		if (strncmp(in, tagnames[tag], sz) == 0 &&
		    (in[sz] == ' ' || in[sz] == '>')) {
			in += sz;
			break;
		}
	}
	
	if (tag == TAG__MAX)
		return tag;

	/*
	 * Find any registered attributes until the closing delimiter.
	 * If the tag in general is malformed (e.g., unexpected NUL),
	 * then bail early by returning TAG__MAX.
	 */

	assert(*in == ' ' || *in == '>');
	assert(tag != TAG__MAX);

	while (isspace((unsigned char)*in))
		in++;

	while (*in != '>') {
		for (attr = 0; attr < ATTR__MAX; attr++) {
			sz = strlen(attrs[attr]);
			if (strncmp(in, attrs[attr], sz) == 0 &&
			    in[sz] == '=')
				break;
		}

		if (attr == ATTR__MAX) {
			for ( ; in[sz] != '\0'; sz++)
				if (in[sz] == '=')
					break;
			if (in[sz] == '\0')
				return TAG__MAX;
		}

		/*
		 * Handle both quoted and unquoted.  Bail out with
		 * TAG__MAX if NUL is encountered.
		 */

		if (in[++sz] == '"') {
			sz++;
			if (attr != ATTR__MAX && outattrs != NULL)
				outattrs[attr] = &in[sz];
			for ( ; in[sz] != '\0'; sz++) {
				if (in[sz] == '"')
					break;
				if (attr != ATTR__MAX &&
				    outattrsz != NULL)
					outattrsz[attr]++;
			}
			if (in[sz] == '\0')
				return TAG__MAX;
			assert(in[sz] == '"');
			sz++;
		} else {
			if (attr != ATTR__MAX && outattrs != NULL)
				outattrs[attr] = &in[sz];
			for (; in[sz] != '\0'; sz++) {
				if (in[sz] == ' ' ||
				    in[sz] == '>')
					break;
				if (attr != ATTR__MAX &&
				    outattrsz != NULL)
					outattrsz[attr]++;
			}
			if (in[sz] == '\0')
				return TAG__MAX;
		}
		in += sz;

		/* Remove trailing spaces. */

		while (isspace((unsigned char)*in))
			in++;
	}

	assert(*in == '>');
	if (outpos != NULL)
		*outpos = (size_t)(++in - start);
	return tag;
}

/*
 * Return non-zero if "new sentence, new line" is in effect, zero
 * otherwise.  Accepts the start and finish offset of a buffer.
 */
static int
newsentence(size_t start, size_t finish, const char *buf)
{
	size_t	 span = finish - start;
	
	assert(finish >= start);

	/* Ignore "i.e." and "e.g.". */

	if ((span >= 4 &&
	     strncasecmp(&buf[finish - 4], "i.e.", 4) == 0) ||
	    (span >= 4 &&
	     strncasecmp(&buf[finish - 4], "e.g.", 4) == 0))
		return 0;

	return 1;
}

/*
 * For the HTML table starting at "buf" and of maximum length "len", try
 * to extract the number of columns.  Returns the number of columns or
 * zero if the columns couldn't be determined.
 */
static size_t
table_columns(const char *buf, size_t len)
{
	size_t		 cols = 0;
	const char	*end, *nbuf, *delim = NULL;

	/* Get the first row.  If found, position within the row. */

	if ((nbuf = memmem(buf, len, "<tr", 3)) == NULL)
		return cols;
	assert(nbuf >= buf);
	len -= (nbuf - buf) + 3;
	buf = nbuf + 3;

	/* Find the next row, which is the end marker. */

	if ((end = memmem(buf, len, "<tr", 3)) == NULL)
		return cols;
	assert(end >= buf);
	if ((len = end - buf) == 0)
		return cols;

	/* See if this row contains <th> or <td> elements. */

	if (memmem(buf, len, "<th", 3) != NULL)
		delim = "<th";
	else if (memmem(buf, len, "<td", 3) != NULL)
		delim = "<td";
	else
		return cols;

	/* Count the row's <th> or <td> elements. */

	assert(delim != NULL);
	for (;;) {
		if ((nbuf = memmem(buf, len, delim, 3)) == NULL)
			break;
		assert(nbuf >= buf);
		len -= (nbuf - buf) + 3;
		buf = nbuf + 3;
		cols++;
	}

	return cols;
}

void
print_description(FILE *f, const struct defn *d)
{
	size_t		 sz, descsz, i, j, col, stripspace, outpos;
	enum tag	 tag;
	int		 incolumn = 0, inblockquote = 0, close;
	const char	*attrs[ATTR__MAX];
	size_t		 attrsz[ATTR__MAX];
	unsigned int	 flags;

	/*
	 * Strip the crap out of the description.
	 * "Crap" consists of things I don't understand that mess up
	 * parsing of the HTML, for instance,
	 *   <dl>[[foo bar]]<dt>foo bar</dt>...</dl>
	 * These are not well-formed HTML.
	 * Note that d->desc[d->descz] is the NUL terminator, so we
	 * don't need to check d->descsz - 1.
	 */

	descsz = d->descsz;
	for (i = 0; i < descsz; ) {
		if (d->desc[i] == '^' &&
		    d->desc[i + 1] == '(') {
			memmove(&d->desc[i],
				&d->desc[i + 2],
				descsz - i - 1);
			descsz -= 2;
			continue;
		} else if (d->desc[i] == ')' &&
			   d->desc[i + 1] == '^') {
			memmove(&d->desc[i],
				&d->desc[i + 2],
				descsz - i - 1);
			descsz -= 2;
			continue;
		} else if (d->desc[i] == '^') {
			memmove(&d->desc[i],
				&d->desc[i + 1],
				descsz - i);
			descsz -= 1;
			continue;
		} else if (d->desc[i] != '[' ||
			   d->desc[i + 1] != '[') {
			i++;
			continue;
		}

		for (j = i; j < descsz; j++)
			if (d->desc[j] == ']' &&
			    d->desc[j + 1] == ']')
				break;

		/* Ignore if we don't have a terminator. */

		assert(j > i);
		j += 2;
		if (j > descsz) {
			i++;
			continue;
		}

		memmove(&d->desc[i], &d->desc[j], descsz - j + 1);
		descsz -= (j - i);
	}

	/*
	 * Here we go!
	 * Print out the description as best we can.
	 * Do on-the-fly processing of any HTML we encounter into
	 * mdoc(7) and try to break lines up.
	 */

	col = stripspace = 0;

	for (i = 0; i < descsz; ) {
		/*
		 * The "stripspace" variable is set to >=2 if we've
		 * stripped white-space off before an anticipated macro.
		 * Without it, if the macro ends up *not* being a macro,
		 * we wouldn't flush the line and thus end up losing a
		 * space.  This lets the code that flushes the line know
		 * that we've stripped spaces and adds them back in.
		 */

		if (stripspace > 0)
			stripspace--;

		/* Ignore NUL byte, just in case. */

		if (d->desc[i] == '\0') {
			i++;
			continue;
		}

		/*
		 * Newlines are paragraph breaks.
		 * If we have multiple newlines, then keep to a single
		 * `Pp' to keep it clean.
		 * Only do this if we're not before a block-level HTML,
		 * as this would mean, for instance, a `Pp'-`Bd' pair.
		 */

		if (d->desc[i] == '\n') {
			while (isspace((unsigned char)d->desc[i]))
				i++;
			tag = parse_tags(&d->desc[i], NULL, NULL, NULL,
				&close);
			if (tag == TAG__MAX ||
			    (close &&
			     (tags[tag].cflags & TAGINFO_INLINE)) ||
			    (!close &&
			     (tags[tag].oflags & TAGINFO_INLINE))) {
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

		if (d->desc[i] == ' ' &&
		    i > 0 && d->desc[i - 1] == '.') {
			for (j = i - 1; j > 0; j--)
				if (isspace((unsigned char)d->desc[j])) {
					j++;
					break;
				}
			if (newsentence(j, i, d->desc)) {
				while (d->desc[i] == ' ')
					i++;
				fputc('\n', f);
				col = 0;
				continue;
			}
		}

		/*
		 * After 65 characters, force a break when we encounter
		 * white-space to keep our lines more or less tidy.
		 */

		if (col > 65 && d->desc[i] == ' ') {
			while (d->desc[i] == ' ' )
				i++;
			fputc('\n', f);
			col = 0;
			continue;
		}

		/* Parse HTML tags and links. */

		if (d->desc[i] == '<' && d->desc[i + 1] != '<' &&
		    (tag = parse_tags(&d->desc[i], &outpos, attrs,
		     attrsz, &close)) != TAG__MAX) {
			/* Valid HTML tag. */

			switch (tag) {
			case TAG_A:
				if (close) {
					fputs("\"\n", f);
					col = 0;
					break;
				}
				if (col > 0)
					fputs("\n", f);
				fputs(".Lk ", f);
				if (attrsz[ATTR_HREF] > 0)
					fprintf(f, "%.*s",
						(int)attrsz[ATTR_HREF],
						attrs[ATTR_HREF]);
				fputs(" \"", f);
				col = 1;
				stripspace = 0;
				break;
			case TAG_BLOCK:
				inblockquote = close ? 0 : 1;
				break;
			case TAG_TD:
				/* FALLTHROUGH */
			case TAG_TH:
				if (close)
					break;
				if (incolumn) {
					if (col > 0)
						fputs("\n", f);
					fputs("T}\t", f);
				}
				fputs("T{\n", f);
				col = 0;
				incolumn = 1;
				break;
			case TAG_TR:
				if (close || !incolumn)
					break;
				if (col > 0)
					fputs("\n", f);
				fputs("T}\n", f);
				col = 0;
				incolumn = 0;
				break;
			case TAG_TABLE:
				if (!close && !inblockquote) {
					if (col > 0)
						fputs("\n", f);
					fputs(".sp\n", f);
					col = 0;
				} else if (close && incolumn) {
					if (col > 0)
						fputs("\n", f);
					fputs("T}\n", f);
					col = 0;
					incolumn = 0;
				}
				break;
			default:
				break;
			}

			i += outpos;
			flags = close ? tags[tag].cflags :
				tags[tag].oflags;

			/*
			 * NOOP tags don't do anything, such as the case
			 * of `</dd>', which only serves to end an `It'
			 * block that will be closed out by a subsequent
			 * `It' or end of clause `El' anyway.  Skip the
			 * trailing space.
			 */

			if (flags == TAGINFO_NOOP) {
				while (isspace((unsigned char)d->desc[i]))
					i++;
			} else if (flags == TAGINFO_INLINE) {
				while (stripspace > 0) {
					fputc(' ', f);
					col++;
					stripspace--;
				}
				if (close)
					fputs(tags[tag].cmdoc, f);
				else
					fputs(tags[tag].omdoc, f);
			} else {
				/*
				 * A breaking mdoc(7) statement.  Break
				 * the current line, output the macro,
				 * and conditionally break following
				 * that (or we might do nothing at all).
				 */

				if (col > 0) {
					fputs("\n", f);
					col = 0;
				}

				if (close)
					fputs(tags[tag].cmdoc, f);
				else
					fputs(tags[tag].omdoc, f);
				if (!(flags & TAGINFO_NOBR)) {
					fputs("\n", f);
					col = 0;
				} else if (!(flags & TAGINFO_NOSP)) {
					fputs(" ", f);
					col++;
				}
				while (isspace((unsigned char)d->desc[i]))
					i++;

				if (tag == TAG_TABLE && close) {
					if (!inblockquote)
						fputs(".sp\n", f);
					col = 0;
				}

				/*
				 * Special-casing of tables, which need
				 * to know the number of subsequent
				 * columns to produce the tbl(7) header.
				 * If the number of columns can't be
				 * determined, don't produce a header,
				 * which will probably result in an ugly
				 * table.
				 */

				if (tag == TAG_TABLE && !close) {
					sz = table_columns(&d->desc[i],
						descsz - i);
					for (j = 0; j < sz; j++)
						fprintf(f, "%sl", j > 0 ?
							" " : "");
					fputs(".\n", f);
				}
			}

			stripspace = 0;
			continue;
		} else if (d->desc[i] == '<' && d->desc[i + 1] == '<') {
			/* Literal '<<' as in bit-shifting. */

			while (stripspace > 0) {
				fputc(' ', f);
				col++;
				stripspace--;
			}
		} else if (d->desc[i] == '[' && d->desc[i + 1] != ']') {
			/* Do we start at the bracket or bar? */

			for (sz = i + 1; sz < descsz; sz++)
				if (d->desc[sz] == '|' ||
				    d->desc[sz] == ']')
					break;

			/* This is a degenerate case. */

			if (sz == descsz) {
				i++;
				stripspace = 0;
				continue;
			}

			/*
			 * Look for a trailing "()", using "j" as a
			 * sentinel in case it was found.  This lets us
			 * print out a "Fn xxxx" instead of having the
			 * function be ugly.  If we don't have a Fn and
			 * we'd stripped space before this, remember to
			 * add the space back in.
			 */

			j = 0;
			if (d->desc[sz] != '|') {
				i = i + 1;
				if (sz > 2 &&
				    d->desc[sz - 1] == ')' &&
				    d->desc[sz - 2] == '(') {
					if (col > 0)
						fputc('\n', f);
					fputs(".Fn ", f);
					j = sz - 2;
					assert(j > 0);
				} else if (stripspace) {
					fputc(' ', f);
					col++;
				}
			} else {
				if (stripspace) {
					fputc(' ', f);
					col++;
				}
				i = sz + 1;
			}

			while (isspace((unsigned char)d->desc[i]))
				i++;

			/*
			 * Now handle in-page references.  If we're a
			 * function reference (e.g., function()), then
			 * omit the trailing parentheses and put in a Fn
			 * block.  Otherwise print them out as-is: we've
			 * already accumulated them into our "SEE ALSO"
			 * values, which we'll use below.
			 */

			for ( ; i < descsz; i++, col++) {
				if (j > 0 && i == j) {
					i += 3;
					for ( ; i < descsz; i++)
						if (d->desc[i] == '.')
							fputs(" .", f);
						else if (d->desc[i] == ',')
							fputs(" ,", f);
						else if (d->desc[i] == ')')
							fputs(" )", f);
						else
							break;

					/* Trim trailing space. */

					while (i < descsz &&
					       isspace((unsigned char)d->desc[i]))
						i++;	

					fputc('\n', f);
					col = 0;
					break;
				} else if (d->desc[i] == ']') {
					i++;
					break;
				}
				fputc(d->desc[i], f);
				col++;
			}

			stripspace = 0;
			continue;
		}

		/* Strip leading spaces from output. */

		if (d->desc[i] == ' ' && col == 0) {
			while (d->desc[i] == ' ')
				i++;
			continue;
		}

		/*
		 * Strip trailing spaces from output.
		 * Set "stripspace" to be the number of white-space
		 * characters that we've skipped, plus one.
		 * This means that the next loop iteration while get the
		 * actual amount we've skipped (for '<' or '[') and we
		 * can act upon it there.
		 */
		
		if (d->desc[i] == ' ') {
			j = i;
			while (j < descsz && d->desc[j] == ' ')
				j++;
			if (j < descsz &&
			    (d->desc[j] == '\n' ||
			     d->desc[j] == '<' ||
			     d->desc[j] == '[')) {
				stripspace = d->desc[j] != '\n' ?
					(j - i + 1) : 0;
				i = j;
				continue;
			}
		}

		assert(d->desc[i] != '\n');

		/*
		 * Handle some oddities.
		 * The following HTML escapes exist in the output that I
		 * could find.
		 * There might be others...
		 */

		if (strncmp(&d->desc[i], "&rarr;", 6) == 0) {
			i += 6;
			fputs("\\(->", f);
		} else if (strncmp(&d->desc[i], "&larr;", 6) == 0) {
			i += 6;
			fputs("\\(<-", f);
		} else if (strncmp(&d->desc[i], "&nbsp;", 6) == 0) {
			i += 6;
			fputc(' ', f);
		} else if (strncmp(&d->desc[i], "&lt;", 4) == 0) {
			i += 4;
			fputc('<', f);
		} else if (strncmp(&d->desc[i], "&gt;", 4) == 0) {
			i += 4;
			fputc('>', f);
		} else if (strncmp(&d->desc[i], "&#91;", 5) == 0) {
			i += 5;
			fputc('[', f);
		} else {
			/* Make sure we don't trigger a macro. */
			if (col == 0 &&
			    (d->desc[i] == '.' || d->desc[i] == '\''))
				fputs("\\&", f);
			fputc(d->desc[i], f);
			i++;
		}

		col++;
	}

	if (col > 0)
		fputs("\n", f);
}
