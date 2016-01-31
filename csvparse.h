
// Simple CSV Parser

#ifndef CSVPARSE_HEADER_INCLUDED_
#define CSVPARSE_HEADER_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#ifndef CHARTYPE
#define CHARTYPE char
#endif

/* CSV Specification
 *
 *  A "line" is specified as one or more "entries" separated by a comma.
 *
 *  An "entry" is a sequence of characters encoding a string of data.
 *
 *  If the first and last character of the entry are quotation marks, the data
 *  encoded by the entry are exactly the characters between the two quotation
 *  marks, excluding the marks themselves.
 *      * The entry is known as a "quoted entry"
 *
 *  A sequence of two adjacent quotation marks encode a single quotation mark.
 *      * This is known as an "escaped quote" or "double quote"
 *
 *  A comma within a quoted entry encodes a comma in the resulting data.
 *
 *  Spaces at the start and end of a non-quoted entry are ignored.
 */

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

// user data should use enums for by-name access

struct csv_record {
    size_t nfields;
    CHARTYPE** fields;
};

struct csv {
    size_t nrecords;
    size_t nfields;
    size_t allocd_records;
    struct csv_record* header;
    struct csv_record** records;
};

struct csv* csv_alloc(void);
struct csv* csv_openpath(const CHARTYPE* path, BOOL hasheader);
struct csv* csv_readfile(FILE* fp, BOOL hasheader);
struct csv* csv_fromstr(const CHARTYPE* str, BOOL isheader);

void csv_free(struct csv*);

/* resize the internal records array for faster loading (fewer allocations) */
void csv_reserve(struct csv* csv, size_t newsize);

void csv_append(struct csv* csv, const CHARTYPE* str);
void csv_appendpath(struct csv* csv, const CHARTYPE* path, BOOL ignoreheader);
void csv_appendfile(struct csv* csv, FILE* fp, BOOL ignoreheader);

struct csv_record* csv_header(struct csv* csv);
struct csv_record* csv_index(struct csv* csv, size_t index);

/* csv_num_fields(line) -> number of fields encoded by line */
size_t csv_num_fields(const CHARTYPE* line);

/* csv_split(line) -> array of strings containing the encoded fields
 * invariant: length of array is csv_num_fields(line) + 1 (for NULL) */
CHARTYPE** csv_split(const CHARTYPE* line);

/* csv_split1(line, [out] newpos) -> the first field encoded by line, with
 * newpos pointing to the character that terminated the field */
CHARTYPE* csv_split1(const CHARTYPE* line, const CHARTYPE** newpos);

#endif // defined CSVPARSE_HEADER_INCLUDED_

