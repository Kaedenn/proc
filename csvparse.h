
// Simple CSV Parser

#ifndef CSVPARSE_HEADER_INCLUDED_
#define CSVPARSE_HEADER_INCLUDED_

#include <stdint.h>

#ifndef LIT
#define LIT(s) (s)
#endif

#ifndef CHAR
#define CHAR char
#endif

#ifndef STRLEN
#define STRLEN strlen
#endif

#ifndef FPRINTF
#define FPRINTF fprintf
#endif

#ifndef STDERR_FOBJ
#define STDERR_FOBJ stderr
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

/* generic APIs: split delimiter-separated-value formats */

/* passing '\0' for the quote char disables quoting altogether */
const CHAR* tok_dsv(const CHAR* line, const CHAR** out, CHAR quot, CHAR delim);
const CHAR** split_dsv(const CHAR* line, CHAR quot, CHAR delim);

/* specific APIs: split actual CSV and PSV (pipe) formats */

const CHAR** parse_csv(const CHAR* line); /* uses actual CSV grammar */
const CHAR** parse_psv(const CHAR* line); /* uses pipes */

struct csv {
    struct csv_opts {
        CHAR quot;
        CHAR delim;
        bool header;
    } opts;
};

struct csv* parse_file(const CHAR* path);

#endif // defined CSVPARSE_HEADER_INCLUDED_

