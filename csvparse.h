
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

/* RFC 4180 "Common Format and MIME Type for CSV Files" defines the following
 * EBNF grammar for the CSV format:

   file = [header CRLF] record *(CRLF record) [CRLF]

   header = name *(COMMA name)

   record = field *(COMMA field)

   name = field

   field = (escaped / non-escaped)

   escaped = DQUOTE *(TEXTDATA / COMMA / CR / LF / 2DQUOTE) DQUOTE

   non-escaped = *TEXTDATA

   COMMA = %x2C ; encoded in C as ','
   CR = %x0D    ; encoded in C as '\r'
   DQUOTE =  %x22   ; encoded in C as '"'
   LF = %x0A    ; encoded in C as '\n'
   CRLF = CR LF ; encoded in C as "\r\n"
   TEXTDATA =  %x20-21 / %x23-2B / %x2D-7E
*/

/* utility APIs */

/* estimate how many entries the line may have */
size_t estimate_num_entries(const TMCHAR* line, TMCHAR delim);

/* generic APIs: split delimiter-separated-value formats */

/* passing '\0' for the quote char disables quoting altogether */

/* strtok replacement: parse line, store the next parsed entry in out, and
 * return the new offset of parsing. Call this repeatedly to split a line.
 *
 * Example usage:
 *
 * const TMCHAR* input = <text input string>;
 *
 * size_t ncols = estimate_num_entries(input, ',');
 * TMCHAR **results = calloc(sizeof(TMCHAR*), ncols);
 * size_t idx = 0;
 * const TMCHAR* temp = input;
 * while (temp && *temp) {
 *     temp = tok_dsv(temp, &results[idx], '"', ',');
 *     ++idx;
 * }
 *
 */
const TMCHAR* tok_dsv(const TMCHAR* line, const TMCHAR** out, TMCHAR quot, TMCHAR delim);

/* convenience function: split line into a NULL-terminated array of strings */
const TMCHAR** split_dsv(const TMCHAR* line, TMCHAR quot, TMCHAR delim);

/* specific APIs: split actual CSV and PSV (pipe) formats */

const TMCHAR** parse_csv(const TMCHAR* line); /* uses actual CSV grammar */
const TMCHAR** parse_psv(const TMCHAR* line); /* uses pipes */

/* specific APIs: write items to proper CSV and PSV formats */

enum Quoting {
    QUOTE_NECESSARY,
    QUOTE_ALL,
    QUOTE_NONE
};

const TMCHAR* format_dsv(const TMCHAR** entries, enum Quoting quote_style,
                         TMCHAR quote, TMCHAR delim);

const TMCHAR* format_csv(const TMCHAR** entries);
const TMCHAR* format_psv(const TMCHAR** entries);

void write_csv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** csv);
void write_psv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** psv);

/* TODO:
 * const TMCHAR* line = FORMAT_CSV(opts.pidm, opts.id, opts.name, ...);
 */

/* Layout
 *
 * struct csv csv = {0};
 * csv.opts.quot = '"';
 * csv.opts.delim = ',';
 * csv.opts.header = false;
 * csv.ncols = 4;
 * csv.size = 0;
 * csv.capacity = 20;
 * csv.records = calloc(sizeof(struct csv_record), csv.capacity);
 *
 * If a row has more entries than the number of configured columns,
 * then the extra entries are dropped.
 *
 * If a row has fewer entries than the number of configured columns,
 * then the missing entries are set to NULL.
 *
 * (This ensures every record is of the same length)
 *
 * The default (NULL) allocator uses a power-of-two algorithm; once
 * capacity is reached, the capacity is doubled. This is referred to by
 * the csv_expand typedef, as csv_obj.expand_fn.
 *
 */

#if 0

typedef size_t(csv_expand)(size_t bytes);

struct csv_options {
    TMCHAR quot;
    TMCHAR delim;
    bool header;
};

struct csv_record {
    TMCHAR** entries;
};

struct csv {
    struct csv_options opts;
    size_t ncols;
    size_t size;
    size_t capacity;
    struct csv_record header;
    struct csv_record* records;
    csv_expand *expand_fn;
};

/* constructors */

struct csv* csv_alloc(struct csv_options* opts, size_t ncols, size_t capacity);
struct csv* parse_file(const TMCHAR* path);

/* allocate a proper CSV struct with initial configuration */
struct csv* csv_alloc_csv(size_t ncols, size_t capacity);

/* allocate a proper PSV (pipe) struct with initial configuration */
struct csv* csv_alloc_psv(size_t ncols, size_t capacity);

/* destructor */

void csv_free(struct csv* csv);

/* mutators */

/* set or update the headers */
void csv_set_header(struct csv* csv, struct csv_record* record);
void csv_parse_header(struct csv* csv, const TMCHAR* line);

/* parse a line and append it to the records stored in the csv, expanding the
 * capacity if necessary */
void csv_parse_append(struct csv* csv, const TMCHAR* line);
void csv_append(struct csv* csv, struct csv_record* record);

/* accessors */

/* obtain the name of the column specified (or NULL if no headers) */
const TMCHAR* csv_header(struct csv* csv, size_t col);

/* obtain the content of the entry at the row and col given */
const TMCHAR* csv_entry(struct csv* csv, size_t row, size_t col);

/* obtain the content of the entry at the row and col name given */
const TMCHAR* csv_entry_byname(struct csv* csv, size_t row, const TMCHAR* name);

#endif

#endif // defined CSVPARSE_HEADER_INCLUDED_

