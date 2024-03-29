/* The MIT License (MIT)
 * 
 * Copyright (c) 2018 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "textcodec/m_textcodec_int.h"

/* The static mappings are put into a hashtable to aid with lookup. For all but very
 * short strings the lookup will be faster when creating the hashtable than looping
 * through the static mapping for every characters.
 *
 * The mapping hashtables are created as multi value to aid with best fit matching.
 * Best fit matches must always be at the end of the mapping so their utf-8 values
 * are after the mapped value and will not be used. Best match is mainly going from
 * utf-8 to to the code page of choice.
 *
 * Best fit matching does not allow multiple character conversions. It is a single
 * code point mapping. For example, '...' as one utf-8 character cannot be represented
 * in ASCII because the ASCII equivelent "..." requires three characters.
 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_textcodec_error_t M_textcodec_encode_cp_map(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler, M_textcodec_cp_map_t *cp_map)
{
	M_hash_u64u64_t     *map;
	const char          *next = in;
	M_textcodec_error_t  res = M_TEXTCODEC_ERROR_SUCCESS;
	size_t               i;

	/* Create our lookup. */
	map = M_hash_u64u64_create(512, 75, M_HASH_U64U64_MULTI_VALUE);
	for (i=0; cp_map[i].descr!=NULL; i++) {
		M_hash_u64u64_insert(map, cp_map[i].ucode, cp_map[i].cp);
	}

	while (*next != '\0' && !M_textcodec_error_is_error(res)) {
		M_uint64       u64v;
		M_uint32       ucode;
		unsigned char  cp;
		M_utf8_error_t ures;

		/* read the next utf8 character. */
		ures = M_utf8_get_cp(next, &ucode, &next);

		/* If we have an invalid we need to skip it. Since utf8 characters
 		 * can have multiple bytes we want to and replacement per cha cater
		 * not per byte. */
		if (ures != M_UTF8_ERROR_SUCCESS) {
			next = M_utf8_next_chr(next);
		}

		if (ures == M_UTF8_ERROR_SUCCESS && M_hash_u64u64_get(map, ucode, &u64v)) {
			cp = (unsigned char)u64v;
			M_textcodec_buffer_add_byte(buf, cp);
		} else {
			/* Either we encountered an invalid utf8 sequence or it's not in the map. */
			switch (ehandler) {
				case M_TEXTCODEC_EHANDLER_FAIL:
					res = M_TEXTCODEC_ERROR_FAIL;
					break;
				case M_TEXTCODEC_EHANDLER_REPLACE:
					M_textcodec_buffer_add_byte(buf, M_CP_REPLACE);
					res = M_TEXTCODEC_ERROR_SUCCESS_EHANDLER;
					break;
				case M_TEXTCODEC_EHANDLER_IGNORE:
					res = M_TEXTCODEC_ERROR_SUCCESS_EHANDLER;
					break;
			}
		}
	}

	M_hash_u64u64_destroy(map);
	return res;
}

M_textcodec_error_t M_textcodec_decode_cp_map(M_textcodec_buffer_t *buf, const char *in, M_textcodec_ehandler_t ehandler, M_textcodec_cp_map_t *cp_map)
{
	M_hash_u64u64_t     *map;
	M_textcodec_error_t  res  = M_TEXTCODEC_ERROR_SUCCESS;
	size_t               len;
	size_t               i;

	/* Create our lookup. */
	map = M_hash_u64u64_create(512, 75, M_HASH_U64U64_MULTI_VALUE);
	for (i=0; cp_map[i].descr!=NULL; i++) {
		M_hash_u64u64_insert(map, cp_map[i].cp, cp_map[i].ucode);
	}

	len = M_str_len(in);
	for (i=0; i<len; i++) {
		unsigned char  c = (unsigned char)in[i];
		M_uint64       u64v;
		char           ubuf[16];
		M_utf8_error_t ures     = M_UTF8_ERROR_SUCCESS;
		size_t         ulen     = 0;
		M_bool         have;

		have = M_hash_u64u64_get(map, c, &u64v);
		if (have) {
			ures = M_utf8_from_cp(ubuf, sizeof(ubuf), &ulen, (M_uint32)u64v);
		}

		if (have && ures == M_UTF8_ERROR_SUCCESS) {
			M_textcodec_buffer_add_bytes(buf, (unsigned char *)ubuf, ulen);
		} else {
			switch (ehandler) {
				case M_TEXTCODEC_EHANDLER_FAIL:
					res = M_TEXTCODEC_ERROR_FAIL;
					break;
				case M_TEXTCODEC_EHANDLER_REPLACE:
					M_textcodec_buffer_add_str(buf, M_UTF8_REPLACE);
					/* Fall-thru */
				case M_TEXTCODEC_EHANDLER_IGNORE:
					res = M_TEXTCODEC_ERROR_SUCCESS_EHANDLER;
					break;
			}
		}

		if (M_textcodec_error_is_error(res)) {
			break;
		}
	}

	M_hash_u64u64_destroy(map);
	return res;
}
