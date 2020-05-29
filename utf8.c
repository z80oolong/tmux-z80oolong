/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicholas.marriott@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "tmux.h"

#ifndef NO_USE_UTF8CJK
/*
 * This is an implementation of wcwidth() and wcswidth() (defined in
 * IEEE Std 1002.1-2001) for Unicode.
 *
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcswidth.html
 *
 * In fixed-width output devices, Latin characters all occupy a single
 * "cell" position of equal width, whereas ideographic CJK characters
 * occupy two such cells. Interoperability between terminal-line
 * applications and (teletype-style) character terminals using the
 * UTF-8 encoding requires agreement on which character should advance
 * the cursor by how many cell positions. No established formal
 * standards exist at present on which Unicode character shall occupy
 * how many cell positions on character terminals. These routines are
 * a first attempt of defining such behavior based on simple rules
 * applied to data provided by the Unicode Consortium.
 *
 * For some graphical characters, the Unicode standard explicitly
 * defines a character-cell width via the definition of the East Asian
 * FullWidth (F), Wide (W), Half-width (H), and Narrow (Na) classes.
 * In all these cases, there is no ambiguity about which width a
 * terminal shall use. For characters in the East Asian Ambiguous (A)
 * class, the width choice depends purely on a preference of backward
 * compatibility with either historic CJK or Western practice.
 * Choosing single-width for these characters is easy to justify as
 * the appropriate long-term solution, as the CJK practice of
 * displaying these characters as double-width comes from historic
 * implementation simplicity (8-bit encoded characters were displayed
 * single-width and 16-bit ones double-width, even for Greek,
 * Cyrillic, etc.) and not any typographic considerations.
 *
 * Much less clear is the choice of width for the Not East Asian
 * (Neutral) class. Existing practice does not dictate a width for any
 * of these characters. It would nevertheless make sense
 * typographically to allocate two character cells to characters such
 * as for instance EM SPACE or VOLUME INTEGRAL, which cannot be
 * represented adequately with a single-width glyph. The following
 * routines at present merely assign a single-cell width to all
 * neutral characters, in the interest of simplicity. This is not
 * entirely satisfactory and should be reconsidered before
 * establishing a formal standard in this area. At the moment, the
 * decision which Not East Asian (Neutral) characters should be
 * represented by double-width glyphs cannot yet be answered by
 * applying a simple rule from the Unicode database content. Setting
 * up a proper standard for the behavior of UTF-8 character terminals
 * will require a careful analysis not only of each Unicode character,
 * but also of each presentation form, something the author of these
 * routines has avoided to do so far.
 *
 * http://www.unicode.org/unicode/reports/tr11/
 *
 * Markus Kuhn -- 2007-05-26 (Unicode 5.0)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

// Delete duplicated '#include <wchar.h>' by Z.OOL. <zool@zool.jpn.org>
//#include <wchar.h>

struct interval {
  int first;
  int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(wchar_t ucs, const struct interval *table, int max) {
  int min = 0;
  int mid;

  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }

  return 0;
}


/* The following two functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - SOFT HYPHEN (U+00AD) has a column width of 1.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      Full-width (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

int mk_wcwidth(wchar_t ucs)
{
  /* sorted list of non-overlapping intervals of non-spacing characters */
  /* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
  static const struct interval combining[] = {
    { 0x0300, 0x036F }, { 0x0483, 0x0486 }, { 0x0488, 0x0489 },
    { 0x0591, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
    { 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 }, { 0x0600, 0x0603 },
    { 0x0610, 0x0615 }, { 0x064B, 0x065E }, { 0x0670, 0x0670 },
    { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
    { 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
    { 0x07A6, 0x07B0 }, { 0x07EB, 0x07F3 }, { 0x0901, 0x0902 },
    { 0x093C, 0x093C }, { 0x0941, 0x0948 }, { 0x094D, 0x094D },
    { 0x0951, 0x0954 }, { 0x0962, 0x0963 }, { 0x0981, 0x0981 },
    { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD },
    { 0x09E2, 0x09E3 }, { 0x0A01, 0x0A02 }, { 0x0A3C, 0x0A3C },
    { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D },
    { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC },
    { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD },
    { 0x0AE2, 0x0AE3 }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
    { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
    { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
    { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
    { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBC, 0x0CBC },
    { 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
    { 0x0CE2, 0x0CE3 }, { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D },
    { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 },
    { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E },
    { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC },
    { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 },
    { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E },
    { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 },
    { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 },
    { 0x1032, 0x1032 }, { 0x1036, 0x1037 }, { 0x1039, 0x1039 },
    { 0x1058, 0x1059 }, { 0x1160, 0x11FF }, { 0x135F, 0x135F },
    { 0x1712, 0x1714 }, { 0x1732, 0x1734 }, { 0x1752, 0x1753 },
    { 0x1772, 0x1773 }, { 0x17B4, 0x17B5 }, { 0x17B7, 0x17BD },
    { 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x17DD, 0x17DD },
    { 0x180B, 0x180D }, { 0x18A9, 0x18A9 }, { 0x1920, 0x1922 },
    { 0x1927, 0x1928 }, { 0x1932, 0x1932 }, { 0x1939, 0x193B },
    { 0x1A17, 0x1A18 }, { 0x1B00, 0x1B03 }, { 0x1B34, 0x1B34 },
    { 0x1B36, 0x1B3A }, { 0x1B3C, 0x1B3C }, { 0x1B42, 0x1B42 },
    { 0x1B6B, 0x1B73 }, { 0x1DC0, 0x1DCA }, { 0x1DFE, 0x1DFF },
    { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x2063 },
    { 0x206A, 0x206F }, { 0x20D0, 0x20EF }, { 0x302A, 0x302F },
    { 0x3099, 0x309A }, { 0xA806, 0xA806 }, { 0xA80B, 0xA80B },
    { 0xA825, 0xA826 }, { 0xFB1E, 0xFB1E }, { 0xFE00, 0xFE0F },
    { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF }, { 0xFFF9, 0xFFFB },
    { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
    { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
    { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
    { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
    { 0xE0100, 0xE01EF }
  };

  /* test for 8-bit control characters */
  if (ucs == 0)
    return 0;
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return -1;

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, combining,
	       sizeof(combining) / sizeof(struct interval) - 1))
    return 0;

  /* if we arrive here, ucs is not a combining or C0/C1 control character */

  return 1 + 
    (ucs >= 0x1100 &&
     (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
      ucs == 0x2329 || ucs == 0x232a ||
      (ucs >= 0x2e80 && ucs <= 0xa4cf &&
       ucs != 0x303f) ||                  /* CJK ... Yi */
      (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
      (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
      (ucs >= 0xfe10 && ucs <= 0xfe19) || /* Vertical forms */
      (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
      (ucs >= 0xff00 && ucs <= 0xff60) || /* Fullwidth Forms */
      (ucs >= 0xffe0 && ucs <= 0xffe6) ||
      (ucs >= 0x20000 && ucs <= 0x2fffd) ||
      (ucs >= 0x30000 && ucs <= 0x3fffd)));
}


int mk_wcswidth(const wchar_t *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}


/*
 * The following functions are the same as mk_wcwidth() and
 * mk_wcswidth(), except that spacing characters in the East Asian
 * Ambiguous (A) category as defined in Unicode Technical Report #11
 * have a column width of 2. This variant might be useful for users of
 * CJK legacy encodings who want to migrate to UCS without changing
 * the traditional terminal character-width behaviour. It is not
 * otherwise recommended for general use.
 */
int mk_wcwidth_cjk(wchar_t ucs)
{
  /* sorted list of non-overlapping intervals of East Asian Ambiguous
   * characters, generated by "uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c" */
  static const struct interval ambiguous[] = {
    { 0x00A1, 0x00A1 }, { 0x00A4, 0x00A4 }, { 0x00A7, 0x00A8 },
    { 0x00AA, 0x00AA }, { 0x00AE, 0x00AE }, { 0x00B0, 0x00B4 },
    { 0x00B6, 0x00BA }, { 0x00BC, 0x00BF }, { 0x00C6, 0x00C6 },
    { 0x00D0, 0x00D0 }, { 0x00D7, 0x00D8 }, { 0x00DE, 0x00E1 },
    { 0x00E6, 0x00E6 }, { 0x00E8, 0x00EA }, { 0x00EC, 0x00ED },
    { 0x00F0, 0x00F0 }, { 0x00F2, 0x00F3 }, { 0x00F7, 0x00FA },
    { 0x00FC, 0x00FC }, { 0x00FE, 0x00FE }, { 0x0101, 0x0101 },
    { 0x0111, 0x0111 }, { 0x0113, 0x0113 }, { 0x011B, 0x011B },
    { 0x0126, 0x0127 }, { 0x012B, 0x012B }, { 0x0131, 0x0133 },
    { 0x0138, 0x0138 }, { 0x013F, 0x0142 }, { 0x0144, 0x0144 },
    { 0x0148, 0x014B }, { 0x014D, 0x014D }, { 0x0152, 0x0153 },
    { 0x0166, 0x0167 }, { 0x016B, 0x016B }, { 0x01CE, 0x01CE },
    { 0x01D0, 0x01D0 }, { 0x01D2, 0x01D2 }, { 0x01D4, 0x01D4 },
    { 0x01D6, 0x01D6 }, { 0x01D8, 0x01D8 }, { 0x01DA, 0x01DA },
    { 0x01DC, 0x01DC }, { 0x0251, 0x0251 }, { 0x0261, 0x0261 },
    { 0x02C4, 0x02C4 }, { 0x02C7, 0x02C7 }, { 0x02C9, 0x02CB },
    { 0x02CD, 0x02CD }, { 0x02D0, 0x02D0 }, { 0x02D8, 0x02DB },
    { 0x02DD, 0x02DD }, { 0x02DF, 0x02DF }, { 0x0391, 0x03A1 },
    { 0x03A3, 0x03A9 }, { 0x03B1, 0x03C1 }, { 0x03C3, 0x03C9 },
    { 0x0401, 0x0401 }, { 0x0410, 0x044F }, { 0x0451, 0x0451 },
    { 0x2010, 0x2010 }, { 0x2013, 0x2016 }, { 0x2018, 0x2019 },
    { 0x201C, 0x201D }, { 0x2020, 0x2022 }, { 0x2024, 0x2027 },
    { 0x2030, 0x2030 }, { 0x2032, 0x2033 }, { 0x2035, 0x2035 },
    { 0x203B, 0x203B }, { 0x203E, 0x203E }, { 0x2074, 0x2074 },
    { 0x207F, 0x207F }, { 0x2081, 0x2084 }, { 0x20AC, 0x20AC },
    { 0x2103, 0x2103 }, { 0x2105, 0x2105 }, { 0x2109, 0x2109 },
    { 0x2113, 0x2113 }, { 0x2116, 0x2116 }, { 0x2121, 0x2122 },
    { 0x2126, 0x2126 }, { 0x212B, 0x212B }, { 0x2153, 0x2154 },
    { 0x215B, 0x215E }, { 0x2160, 0x216B }, { 0x2170, 0x2179 },
    { 0x2190, 0x2199 }, { 0x21B8, 0x21B9 }, { 0x21D2, 0x21D2 },
    { 0x21D4, 0x21D4 }, { 0x21E7, 0x21E7 }, { 0x2200, 0x2200 },
    { 0x2202, 0x2203 }, { 0x2207, 0x2208 }, { 0x220B, 0x220B },
    { 0x220F, 0x220F }, { 0x2211, 0x2211 }, { 0x2215, 0x2215 },
    { 0x221A, 0x221A }, { 0x221D, 0x2220 }, { 0x2223, 0x2223 },
    { 0x2225, 0x2225 }, { 0x2227, 0x222C }, { 0x222E, 0x222E },
    { 0x2234, 0x2237 }, { 0x223C, 0x223D }, { 0x2248, 0x2248 },
    { 0x224C, 0x224C }, { 0x2252, 0x2252 }, { 0x2260, 0x2261 },
    { 0x2264, 0x2267 }, { 0x226A, 0x226B }, { 0x226E, 0x226F },
    { 0x2282, 0x2283 }, { 0x2286, 0x2287 }, { 0x2295, 0x2295 },
    { 0x2299, 0x2299 }, { 0x22A5, 0x22A5 }, { 0x22BF, 0x22BF },
    { 0x2312, 0x2312 }, { 0x2460, 0x24E9 }, { 0x24EB, 0x254B },
    { 0x2550, 0x2573 }, { 0x2580, 0x258F }, { 0x2592, 0x2595 },
    { 0x25A0, 0x25A1 }, { 0x25A3, 0x25A9 }, { 0x25B2, 0x25B3 },
    { 0x25B6, 0x25B7 }, { 0x25BC, 0x25BD }, { 0x25C0, 0x25C1 },
    { 0x25C6, 0x25C8 }, { 0x25CB, 0x25CB }, { 0x25CE, 0x25D1 },
    { 0x25E2, 0x25E5 }, { 0x25EF, 0x25EF }, { 0x2605, 0x2606 },
    { 0x2609, 0x2609 }, { 0x260E, 0x260F }, { 0x2614, 0x2615 },
    { 0x261C, 0x261C }, { 0x261E, 0x261E }, { 0x2640, 0x2640 },
    { 0x2642, 0x2642 }, { 0x2660, 0x2661 }, { 0x2663, 0x2665 },
    { 0x2667, 0x266A }, { 0x266C, 0x266D }, { 0x266F, 0x266F },
    { 0x273D, 0x273D }, { 0x2776, 0x277F }, { 0xE000, 0xF8FF },
    { 0xFFFD, 0xFFFD }, { 0xF0000, 0xFFFFD }, { 0x100000, 0x10FFFD }
  };

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, ambiguous,
	       sizeof(ambiguous) / sizeof(struct interval) - 1))
    return 2;

#if 1
  return wcwidth(ucs);
#else
  return mk_wcwidth(ucs);
#endif
}


int mk_wcswidth_cjk(const wchar_t *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth_cjk(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}
#endif

struct utf8_item {
	u_int			offset;
	RB_ENTRY(utf8_item)	entry;

	char			data[UTF8_SIZE];
	u_char			size;
};
RB_HEAD(utf8_tree, utf8_item);

static int
utf8_cmp(struct utf8_item *ui1, struct utf8_item *ui2)
{
	if (ui1->size < ui2->size)
		return (-1);
	if (ui1->size > ui2->size)
		return (1);
	return (memcmp(ui1->data, ui2->data, ui1->size));
}
RB_GENERATE_STATIC(utf8_tree, utf8_item, entry, utf8_cmp);
static struct utf8_tree utf8_tree = RB_INITIALIZER(utf8_tree);

static struct utf8_item *utf8_list;
static u_int		 utf8_list_size;
static u_int		 utf8_list_used;

union utf8_map {
	utf8_char	uc;
	struct {
		u_char	flags;
#define UTF8_FLAG_SIZE 0x1f
#define UTF8_FLAG_WIDTH2 0x20

		u_char	data[3];
	};
} __packed;

static const union utf8_map utf8_space1 = {
	.flags = 1,
	.data = " "
};
static const union utf8_map utf8_space2 = {
	.flags = UTF8_FLAG_WIDTH2|2,
	.data = "  "
};

/* Get a UTF-8 item by offset. */
static struct utf8_item *
utf8_get_item(const char *data, size_t size)
{
	struct utf8_item	ui;

	memcpy(ui.data, data, size);
	ui.size = size;

	return (RB_FIND(utf8_tree, &utf8_tree, &ui));
}

/* Expand UTF-8 list. */
static int
utf8_expand_list(void)
{
	if (utf8_list_size == 0xffffff)
		return (-1);
	if (utf8_list_size == 0)
		utf8_list_size = 256;
	else if (utf8_list_size > 0x7fffff)
		utf8_list_size = 0xffffff;
	else
		utf8_list_size *= 2;
	utf8_list = xreallocarray(utf8_list, utf8_list_size, sizeof *utf8_list);
	return (0);
}

/* Add a UTF-8 item. */
static int
utf8_put_item(const char *data, size_t size, u_int *offset)
{
	struct utf8_item	*ui;

	ui = utf8_get_item(data, size);
	if (ui != NULL) {
		*offset = ui->offset;
		log_debug("%s: have %.*s at %u", __func__, (int)size, data,
		    *offset);
		return (0);
	}

	if (utf8_list_used == utf8_list_size && utf8_expand_list() != 0)
		return (-1);
	*offset = utf8_list_used++;

	ui = &utf8_list[*offset];
	ui->offset = *offset;
	memcpy(ui->data, data, size);
	ui->size = size;
	RB_INSERT(utf8_tree, &utf8_tree, ui);

	log_debug("%s: added %.*s at %u", __func__, (int)size, data, *offset);
	return (0);
}

#ifndef NO_USE_UTF8CJK
static enum utf8_state utf8_width(struct utf8_data *ud, int *width);
#endif

/* Get UTF-8 character from data. */
enum utf8_state
utf8_from_data(const struct utf8_data *ud, utf8_char *uc)
{
	union utf8_map	 m = { .uc = 0 };
	u_int		 offset;
#ifndef NO_USE_UTF8CJK
	int		 width;
#endif

	if (ud->width != 1 && ud->width != 2)
		fatalx("invalid UTF-8 width");
	if (ud->size == 0)
		fatalx("invalid UTF-8 size");

	if (ud->size > UTF8_FLAG_SIZE)
		goto fail;
	if (ud->size == 1) {
		*uc = utf8_build_one(ud->data[0], 1);
		return (UTF8_DONE);
	}

	m.flags = ud->size;
	if (ud->width == 2)
		m.flags |= UTF8_FLAG_WIDTH2;

	if (ud->size <= 3)
		memcpy(m.data, ud->data, ud->size);
	else {
		if (utf8_put_item(ud->data, ud->size, &offset) != 0)
			goto fail;
		m.data[0] = (offset & 0xff);
		m.data[1] = (offset >> 8) & 0xff;
		m.data[2] = (offset >> 16);
	}
#ifndef NO_USE_UTF8CJK
	(void)utf8_width(ud, &width);
	if (width == 2)
		m.flags |= UTF8_FLAG_WIDTH2;
#endif
	*uc = m.uc;
	return (UTF8_DONE);

fail:
	if (ud->width == 1)
		*uc = utf8_space1.uc;
	else
		*uc = utf8_space2.uc;
	return (UTF8_ERROR);
}

/* Get UTF-8 data from character. */
void
utf8_to_data(utf8_char uc, struct utf8_data *ud)
{
	union utf8_map		 m = { .uc = uc };
	struct utf8_item	*ui;
	u_int			 offset;
#ifndef NO_USE_UTF8CJK
	int			 width;
#endif

	memset(ud, 0, sizeof *ud);
	ud->size = ud->have = (m.flags & UTF8_FLAG_SIZE);
	if (m.flags & UTF8_FLAG_WIDTH2)
		ud->width = 2;
	else
		ud->width = 1;

	if (ud->size <= 3) {
		memcpy(ud->data, m.data, ud->size);
#ifndef NO_USE_UTF8CJK
		(void)utf8_width(ud, &width);
		ud->width = width;
#endif
		return;
	}

	offset = ((u_int)m.data[2] << 16)|((u_int)m.data[1] << 8)|m.data[0];
	if (offset >= utf8_list_used)
		memset(ud->data, ' ', ud->size);
	else {
		ui = &utf8_list[offset];
		memcpy(ud->data, ui->data, ud->size);
	}
#ifndef NO_USE_UTF8CJK
	(void)utf8_width(ud, width);
	ud->width = width;
#endif
}

/* Get UTF-8 character from a single ASCII character. */
u_int
utf8_build_one(char c, u_int width)
{
	union utf8_map	m = { .flags = 1, .data[0] = c };

	if (width == 2)
		m.flags |= UTF8_FLAG_WIDTH2;
	return (m.uc);
}

/* Set a single character. */
void
utf8_set(struct utf8_data *ud, u_char ch)
{
	static const struct utf8_data empty = { { 0 }, 1, 1, 1 };

	memcpy(ud, &empty, sizeof *ud);
	*ud->data = ch;
}

/* Copy UTF-8 character. */
void
utf8_copy(struct utf8_data *to, const struct utf8_data *from)
{
	u_int	i;

	memcpy(to, from, sizeof *to);

	for (i = to->size; i < sizeof to->data; i++)
		to->data[i] = '\0';
}

/* Get width of Unicode character. */
static enum utf8_state
utf8_width(struct utf8_data *ud, int *width)
{
	wchar_t	wc;

	switch (mbtowc(&wc, ud->data, ud->size)) {
	case -1:
		log_debug("UTF-8 %.*s, mbtowc() %d", (int)ud->size, ud->data,
		    errno);
		mbtowc(NULL, NULL, MB_CUR_MAX);
		return (UTF8_ERROR);
	case 0:
		return (UTF8_ERROR);
	}
#ifndef NO_USE_UTF8CJK
	if (options_get_number(global_options, "utf8-cjk")) {
		*width = mk_wcwidth_cjk(wc);
	} else {
		*width = mk_wcwidth(wc);
	}
	log_debug("UTF-8 %.*s, wcwidth() %d", (int)ud->size, ud->data, *width);
#else
	*width = wcwidth(wc);
#endif
	if (*width >= 0 && *width <= 0xff)
		return (UTF8_DONE);
	log_debug("UTF-8 %.*s, wcwidth() %d", (int)ud->size, ud->data, *width);

#ifndef __OpenBSD__
	/*
	 * Many platforms (particularly and inevitably OS X) have no width for
	 * relatively common characters (wcwidth() returns -1); assume width 1
	 * in this case. This will be wrong for genuinely nonprintable
	 * characters, but they should be rare. We may pass through stuff that
	 * ideally we would block, but this is no worse than sending the same
	 * to the terminal without tmux.
	 */
	if (*width < 0) {
		*width = 1;
		return (UTF8_DONE);
	}
#endif
	return (UTF8_ERROR);
}

/*
 * Open UTF-8 sequence.
 *
 * 11000010-11011111 C2-DF start of 2-byte sequence
 * 11100000-11101111 E0-EF start of 3-byte sequence
 * 11110000-11110100 F0-F4 start of 4-byte sequence
 */
enum utf8_state
utf8_open(struct utf8_data *ud, u_char ch)
{
	memset(ud, 0, sizeof *ud);
	if (ch >= 0xc2 && ch <= 0xdf)
		ud->size = 2;
	else if (ch >= 0xe0 && ch <= 0xef)
		ud->size = 3;
	else if (ch >= 0xf0 && ch <= 0xf4)
		ud->size = 4;
	else
		return (UTF8_ERROR);
	utf8_append(ud, ch);
	return (UTF8_MORE);
}

/* Append character to UTF-8, closing if finished. */
enum utf8_state
utf8_append(struct utf8_data *ud, u_char ch)
{
	int	width;

	if (ud->have >= ud->size)
		fatalx("UTF-8 character overflow");
	if (ud->size > sizeof ud->data)
		fatalx("UTF-8 character size too large");

	if (ud->have != 0 && (ch & 0xc0) != 0x80)
		ud->width = 0xff;

	ud->data[ud->have++] = ch;
	if (ud->have != ud->size)
		return (UTF8_MORE);

	if (ud->width == 0xff)
		return (UTF8_ERROR);
	if (utf8_width(ud, &width) != UTF8_DONE)
		return (UTF8_ERROR);
	ud->width = width;

	return (UTF8_DONE);
}

/*
 * Encode len characters from src into dst, which is guaranteed to have four
 * bytes available for each character from src (for \abc or UTF-8) plus space
 * for \0.
 */
int
utf8_strvis(char *dst, const char *src, size_t len, int flag)
{
	struct utf8_data	 ud;
	const char		*start = dst, *end = src + len;
	enum utf8_state		 more;
	size_t			 i;

	while (src < end) {
		if ((more = utf8_open(&ud, *src)) == UTF8_MORE) {
			while (++src < end && more == UTF8_MORE)
				more = utf8_append(&ud, *src);
			if (more == UTF8_DONE) {
				/* UTF-8 character finished. */
				for (i = 0; i < ud.size; i++)
					*dst++ = ud.data[i];
				continue;
			}
			/* Not a complete, valid UTF-8 character. */
			src -= ud.have;
		}
		if (src[0] == '$' && src < end - 1) {
			if (isalpha((u_char)src[1]) ||
			    src[1] == '_' ||
			    src[1] == '{')
				*dst++ = '\\';
			*dst++ = '$';
		} else if (src < end - 1)
			dst = vis(dst, src[0], flag, src[1]);
		else if (src < end)
			dst = vis(dst, src[0], flag, '\0');
		src++;
	}
	*dst = '\0';
	return (dst - start);
}

/* Same as utf8_strvis but allocate the buffer. */
int
utf8_stravis(char **dst, const char *src, int flag)
{
	char	*buf;
	int	 len;

	buf = xreallocarray(NULL, 4, strlen(src) + 1);
	len = utf8_strvis(buf, src, strlen(src), flag);

	*dst = xrealloc(buf, len + 1);
	return (len);
}

/* Does this string contain anything that isn't valid UTF-8? */
int
utf8_isvalid(const char *s)
{
	struct utf8_data ud;
	const char	*end;
	enum utf8_state	 more;

	end = s + strlen(s);
	while (s < end) {
		if ((more = utf8_open(&ud, *s)) == UTF8_MORE) {
			while (++s < end && more == UTF8_MORE)
				more = utf8_append(&ud, *s);
			if (more == UTF8_DONE)
				continue;
			return (0);
		}
		if (*s < 0x20 || *s > 0x7e)
			return (0);
		s++;
	}
	return (1);
}

/*
 * Sanitize a string, changing any UTF-8 characters to '_'. Caller should free
 * the returned string. Anything not valid printable ASCII or UTF-8 is
 * stripped.
 */
char *
utf8_sanitize(const char *src)
{
	char		*dst = NULL;
	size_t		 n = 0;
	enum utf8_state	 more;
	struct utf8_data ud;
	u_int		 i;

	while (*src != '\0') {
		dst = xreallocarray(dst, n + 1, sizeof *dst);
		if ((more = utf8_open(&ud, *src)) == UTF8_MORE) {
			while (*++src != '\0' && more == UTF8_MORE)
				more = utf8_append(&ud, *src);
			if (more == UTF8_DONE) {
				dst = xreallocarray(dst, n + ud.width,
				    sizeof *dst);
				for (i = 0; i < ud.width; i++)
					dst[n++] = '_';
				continue;
			}
			src -= ud.have;
		}
		if (*src > 0x1f && *src < 0x7f)
			dst[n++] = *src;
		else
			dst[n++] = '_';
		src++;
	}
	dst = xreallocarray(dst, n + 1, sizeof *dst);
	dst[n] = '\0';
	return (dst);
}

/* Get UTF-8 buffer length. */
size_t
utf8_strlen(const struct utf8_data *s)
{
	size_t	i;

	for (i = 0; s[i].size != 0; i++)
		/* nothing */;
	return (i);
}

/* Get UTF-8 string width. */
u_int
utf8_strwidth(const struct utf8_data *s, ssize_t n)
{
	ssize_t	i;
	u_int	width = 0;

	for (i = 0; s[i].size != 0; i++) {
		if (n != -1 && n == i)
			break;
		width += s[i].width;
	}
	return (width);
}

/*
 * Convert a string into a buffer of UTF-8 characters. Terminated by size == 0.
 * Caller frees.
 */
struct utf8_data *
utf8_fromcstr(const char *src)
{
	struct utf8_data	*dst = NULL;
	size_t			 n = 0;
	enum utf8_state		 more;

	while (*src != '\0') {
		dst = xreallocarray(dst, n + 1, sizeof *dst);
		if ((more = utf8_open(&dst[n], *src)) == UTF8_MORE) {
			while (*++src != '\0' && more == UTF8_MORE)
				more = utf8_append(&dst[n], *src);
			if (more == UTF8_DONE) {
				n++;
				continue;
			}
			src -= dst[n].have;
		}
		utf8_set(&dst[n], *src);
		n++;
		src++;
	}
	dst = xreallocarray(dst, n + 1, sizeof *dst);
	dst[n].size = 0;
	return (dst);
}

/* Convert from a buffer of UTF-8 characters into a string. Caller frees. */
char *
utf8_tocstr(struct utf8_data *src)
{
	char	*dst = NULL;
	size_t	 n = 0;

	for(; src->size != 0; src++) {
		dst = xreallocarray(dst, n + src->size, 1);
		memcpy(dst + n, src->data, src->size);
		n += src->size;
	}
	dst = xreallocarray(dst, n + 1, 1);
	dst[n] = '\0';
	return (dst);
}

/* Get width of UTF-8 string. */
u_int
utf8_cstrwidth(const char *s)
{
	struct utf8_data	tmp;
	u_int			width;
	enum utf8_state		more;

	width = 0;
	while (*s != '\0') {
		if ((more = utf8_open(&tmp, *s)) == UTF8_MORE) {
			while (*++s != '\0' && more == UTF8_MORE)
				more = utf8_append(&tmp, *s);
			if (more == UTF8_DONE) {
				width += tmp.width;
				continue;
			}
			s -= tmp.have;
		}
		if (*s > 0x1f && *s != 0x7f)
			width++;
		s++;
	}
	return (width);
}

/* Pad UTF-8 string to width on the left. Caller frees. */
char *
utf8_padcstr(const char *s, u_int width)
{
	size_t	 slen;
	char	*out;
	u_int	 n, i;

	n = utf8_cstrwidth(s);
	if (n >= width)
		return (xstrdup(s));

	slen = strlen(s);
	out = xmalloc(slen + 1 + (width - n));
	memcpy(out, s, slen);
	for (i = n; i < width; i++)
		out[slen++] = ' ';
	out[slen] = '\0';
	return (out);
}

/* Pad UTF-8 string to width on the right. Caller frees. */
char *
utf8_rpadcstr(const char *s, u_int width)
{
	size_t	 slen;
	char	*out;
	u_int	 n, i;

	n = utf8_cstrwidth(s);
	if (n >= width)
		return (xstrdup(s));

	slen = strlen(s);
	out = xmalloc(slen + 1 + (width - n));
	for (i = 0; i < width - n; i++)
		out[i] = ' ';
	memcpy(out + i, s, slen);
	out[i + slen] = '\0';
	return (out);
}

int
utf8_cstrhas(const char *s, const struct utf8_data *ud)
{
	struct utf8_data	*copy, *loop;
	int			 found = 0;

	copy = utf8_fromcstr(s);
	for (loop = copy; loop->size != 0; loop++) {
		if (loop->size != ud->size)
			continue;
		if (memcmp(loop->data, ud->data, loop->size) == 0) {
			found = 1;
			break;
		}
	}
	free(copy);

	return (found);
}
