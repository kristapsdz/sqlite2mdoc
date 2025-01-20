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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

static	const char *tags[TAG__MAX] = {
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

enum tag
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
		sz = strlen(tags[tag]);
		assert(sz > 0);
		if (strncmp(in, tags[tag], sz) == 0 &&
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
