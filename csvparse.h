
// Simple CSV Parser

#ifndef CSVPARSE_HEADER_INCLUDED_
#define CSVPARSE_HEADER_INCLUDED_

#include <stdint.h>

#ifdef NON_UA_ISOLATED_TEST

#define TMCHAR char
#define UFILE FILE
#define _TMC(s) s
#define tmstrlen strlen
#define ua_exit exit
#define tmstderr stderr
#define tmfprintf(bundle, fdes, ...) fprintf(fdes, __VA_ARGS__)
#define tmfopen(bundle, fname, mode) fopen(fname, mode)
#define tmfclose(fdes) fclose(fdes)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#else

#if !defined(csvBundle_EXISTS)
  #include "tmcilib.h"
  static struct TMBundle csvBundle = {"csvparse.c", NULL, NULL, NULL, NULL};
  #define csvBundle_EXISTS
#endif

#endif

int strcount(const TMCHAR* s, TMCHAR c);

/* strtok replacement: parse line, store the next parsed entry in out, and
 * return the new offset of parsing. Call this repeatedly to split a line.
 * Using '\0' as the quote char disables quoting altogether.
 *
 * const TMCHAR* input = <text input string>;
 *
 * const int ncols = strcount(input, _TMC(','));
 * TMCHAR **results = calloc(sizeof(TMCHAR*), ncols);
 * size_t idx = 0;
 * const TMCHAR* temp = input;
 * while (temp && *temp) {
 *     temp = dsvtok(temp, &results[idx], '"', ',');
 *     ++idx;
 * }
 *
 */
const TMCHAR* dsvtok(const TMCHAR* line, /* line to tokenize */
                     const TMCHAR** out, /* where to store the results */
                     TMCHAR quot,        /* quoting char ('\0' to disable) */
                     TMCHAR delim);      /* delimiting char */

/* For format_dsv: specify when quoting should be performed. By default, only
 * quote fields when necessary.
 *
 * "Necessary" means quote fields only when a subsequent parsing would yield
 * a result different from the initial input.
 *
 * "All" means quote everything.
 *
 * "None" means quote nothing.
 *
 * "NonNumeric" means quote anything that isn't strictly a number.
 */
enum Quoting {
    QUOTE_NECESSARY = 0,
    QUOTE_ALL,
    QUOTE_NONE,
    QUOTE_NONNUMERIC
};

const TMCHAR* format_dsv(const TMCHAR** entries, enum Quoting quote_style,
                         TMCHAR quote, TMCHAR delim, TMCHAR escape);

/* Generic delimiter-separated-value formatting
 * format_csv === format_dsv(..., QUOTE_NECESSARY, _TMC('"'), _TMC(','), 0);
 * format_psv === format_dsv(..., QUOTE_NONE, 0, _TMC('|'), 0);
 */
const TMCHAR* format_csv(const TMCHAR** entries);
const TMCHAR* format_psv(const TMCHAR** entries);

/* passing '\0' for the quote char disables quoting altogether */

const TMCHAR** parse_dsv(const TMCHAR* line, TMCHAR quot, TMCHAR delim);
const TMCHAR** parse_csv(const TMCHAR* line); /* uses actual CSV grammar */
const TMCHAR** parse_psv(const TMCHAR* line); /* uses pipes */

int write_csv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** csv);
int write_psv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** psv);

int fwrite_csv(UFILE* file, const TMCHAR** csv);
int fwrite_psv(UFILE* file, const TMCHAR** psv);

#endif // defined CSVPARSE_HEADER_INCLUDED_

