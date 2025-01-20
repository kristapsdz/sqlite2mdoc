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
#ifndef EXTERN_H
#define EXTERN_H

/*
 * Phase of parsing input file.
 */
enum	phase {
	PHASE_INIT = 0, /* waiting to encounter definition */
	PHASE_KEYS, /* have definition, now keywords */
	PHASE_DESC, /* have keywords, now description */
	PHASE_SEEALSO,
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
	char		 *name; /* really Nd */
	TAILQ_ENTRY(defn) entries;
	char		 *desc; /* long description */
	size_t		  descsz; /* strlen(desc) */
	char		 *fulldesc; /* description w/newlns */
	size_t		  fulldescsz; /* strlen(fulldesc) */
	struct declq	  dcqhead; /* declarations */
	int		  multiline; /* used when parsing */
	int		  instruct; /* used when parsing */
	const char	 *fn; /* parsed from file */
	size_t		  ln; /* parsed at line */
	int		  postprocessed; /* good for emission? */
	char		 *dt; /* manpage title */
	char		**nms; /* manpage names */
	size_t		  nmsz; /* number of names */
	char		 *fname; /* manpage filename */
	char		 *keybuf; /* raw keywords */
	size_t		  keybufsz; /* length of "keysbuf" */
	char		 *seealso; /* see also tags */
	size_t		  seealsosz; /* length of seealso */
	char		**xrs; /* parsed "see also" references */
	size_t		  xrsz; /* number of references */
	char		**keys; /* parsed keywords */
	size_t		  keysz; /* number of keywords */
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

void	print_description(FILE *, const struct defn *);
void	print_implementation(FILE *, const struct defn *, int);
void	print_synopsis(FILE *, const struct decl *, const struct defn *);

#endif /*!EXTERN_H*/
