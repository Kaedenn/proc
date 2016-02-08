
#ifndef UA_ORAC_GOR2CSV_HEADER_
#define UA_ORAC_GOR2CSV_HEADER_

#if !defined(csvBundle_EXISTS)
#include "tmcilib.h"
static struct TMBundle csvBundle = {"gor2csv.h", NULL, NULL, NULL, NULL};
#define csvBundle_EXISTS
#endif

/** @region Utility functions **/

int ua_strcount(const TMCHAR* s, TMCHAR c);

/** @region Parsing functions **/

/* ua_dsvtok(line, output, quotechar, delimchar)
 *
 * Parse one entry of @param line into @param out, returning the resulting
 * position after the parse.
 *
 * @param line      input text to parse
 * @param out       receives parsed record
 * @param quot      quoting character, use '\0' to disable quoting
 * @param delim     delimiter character
 *
 * Returns NULL on error and a string of length zero on EOL.
 *
 * Example:
 *
 * const TMCHAR* input = <text input string>;
 * TMCHAR quote = <quote>;
 * TMCHAR delim = <delimiter>;
 * TMCHAR** results;
 *
 * const TMCHAR* temp = input;
 * results = calloc(sizeof(TMCHAR*), ua_strcount(input, delim));
 * std::size_t idx = 0;
 * while (temp && *temp) {
 *     temp = ua_dsvtok(input, &results[idx], quote, delim);
 *     idx += 1;
 * }
 */
const TMCHAR* ua_dsvtok(const TMCHAR* line,     /* parse this */
                        const TMCHAR** out,     /* into this */
                        TMCHAR quot,            /* using this quote */
                        TMCHAR delim);          /* and this delim */

const TMCHAR** ua_parse_dsv(const TMCHAR* line, TMCHAR quote, TMCHAR delim);
const TMCHAR** ua_parse_csv(const TMCHAR* line);
const TMCHAR** ua_parse_psv(const TMCHAR* line);

enum UAQuoteStyle {
    QUOTE_NECESSARY = 0,    /* quote only when necessary (see below) */
    QUOTE_ALL,              /* quote everything */
    QUOTE_NONE,             /* disable quoting */
    QUOTE_NONNUMERIC        /* quote things that aren't numbers */
};

/* Quoting
 *
 * When using QUOTE_NECESSARY, a field is enclosed in quotes if any of the
 * following conditions are met:
 *      The field begins or ends with the quote character
 *      The field begins or ends with a space character: ' '
 *      The field contains the delimiting character
 *      The field contains an EOL character: either '\r' or '\n'
 *
 * This is to ensure that a round-trip format-parse-format yields exactly the
 * same data.
 *
 * Passing '\0' as a quote character to any function will disable quoting
 * entirely. This will override a QUOTE_* enumeration.
 */

/** @region Formatting functions **/

const TMCHAR* ua_format_dsv(const TMCHAR** data,
                            enum UAQuoteStyle quoting,
                            TMCHAR quote,
                            TMCHAR delim,
                            TMCHAR escape);

/* ua_write_dsv(path, mode, <format-args>);
 *
 * Calls ua_format_dsv with <format-args> and writes the results to
 * @param path, opened with the mode @param mode.
 *
 * Returns true on success, false on failure.
 */
bool ua_write_dsv(const TMCHAR* path, const TMCHAR* mode,
                  const TMCHAR** data, enum UAQuoteStyle quoting,
                  TMCHAR quote, TMCHAR delim, TMCHAR escape);

/* ua_fwrite_dsv(file, <format-args>)
 *
 * Calls ua_format_dsv with <format-args> and writes the results to
 * @param file.
 *
 * Returns true on success, false on failure.
 */
bool ua_fwrite_dsv(UFILE* file,
                   const TMCHAR** data, enum UAQuoteStyle quoting,
                   TMCHAR quote, TMCHAR delim, TMCHAR escape);

/* ua_format_csv(data)
 *
 * Calls ua_format_dsv(data, QUOTE_NECESSARY, _TMC('"'), _TMC(','), 0);
 */
const TMCHAR* ua_format_csv(const TMCHAR** data);

/* ua_format_csv(data)
 *
 * Calls ua_write_dsv(path, mode, data, QUOTE_NECESSARY,
 *                    _TMC('"'), _TMC('|'), 0);
 */
bool ua_write_csv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data);

/* ua_fwrite_csv(file, data)
 *
 * Calls ua_fwrite_dsv(file, data, QUOTE_NECESSARY, _TMC('"'), _TMC('|'), 0)
 */
bool ua_fwrite_csv(UFILE* file, const TMCHAR** data);

/* ua_format_psv(data)
 *
 * Calls ua_format_dsv(data, QUOTE_NONE, 0, _TMC('|'), 0);
 */
const TMCHAR* ua_format_psv(const TMCHAR** data);

/* ua_write_psv(path, mode, data)
 *
 * Calls ua_write_dsv(path, mode, data, QUOTE_NONE, 0, _TMC('|'), 0);
 */
bool ua_write_psv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data);

/* ua_fwrite_psv(file, data)
 *
 * Calls ua_fwrite_dsv(file, data, QUOTE_NONE, 0, _TMC('|'), 0)
 */
bool ua_fwrite_psv(UFILE* file, const TMCHAR** data);

#endif
