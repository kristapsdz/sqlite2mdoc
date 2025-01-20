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

enum tag
parse_tags(const char *, size_t *, const char **, size_t *, int *);

#endif /*!EXTERN_H*/
