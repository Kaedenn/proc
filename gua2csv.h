
/*****************************************************************************/
/*    Name: gua2csv.h                                                        */
/*   Title: Delimiter-Separated-Value Library                                */
/* Purpose: Provide a simple, correct, and reliable mechanism for            */
/*          interacting with data separated by single-character delimiters.  */
/*  Author: Peter Schultz (sxpws)                                            */
/*****************************************************************************/
/* UA AUDIT TRAIL                                                            */
/*                                                                           */
/* 2016/02/12 sxpws Initial commit                                           */
/* 2016/02/14 sxpws Added ua_free_dsv                                        */
/*                                                                           */
/* UA AUDIT TRAIL END                                                        */
/*****************************************************************************/

#ifndef UA_ORAC_GUA2CSV_HEADER_
#define UA_ORAC_GUA2CSV_HEADER_

#if !defined(csvBundle_EXISTS)
#include "tmcilib.h"
static struct TMBundle csvBundle = {"gua2csv.h", NULL, NULL, NULL, NULL};
#define csvBundle_EXISTS
#endif

#ifdef __cplusplus
extern "C" {
#endif /* def __cplusplus */

/** @region Utility functions **/

/* ua_strcount(string, character)
 *
 * Count the number of times character @param c occurs in string @param s.
 *
 * @param s         string to scan
 * @param c         character to count
 *
 * Returns the number of times @param c occurs in @param s.
 */
int ua_strcount(const TMCHAR* s, TMCHAR c);

/** @region Parsing functions **/

/* ua_dsvtok(line, output, quotechar, delimchar)
 *
 * Parse one entry of @param line into @param out, returning the resulting
 * position after the parse. While technically a private API, this function is
 * exposed under the assumption that it may be useful for extremely large
 * datasets.
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
 * // specify input data, configuration, and declare the output variable
 * const TMCHAR* input = <text input string>;
 * TMCHAR quote = <quote>;
 * TMCHAR delim = <delimiter>;
 * TMCHAR** results;
 *
 * // perform the parsing
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

/* ua_parse_dsv(line, quotechar, delimchar)
 *
 * Parse @param line using the specified quoting character and delimiting
 * character, returning a vector of strings as a result.
 *
 * @param line      input text to parse
 * @param quote     quoting character to use (or '\0' to disable quoting)
 * @param delim     delimiting character to use
 *
 * Returns a NULL-terminated array of NULL-terminated strings or NULL on
 * error. Use ua_free_dsv to free the returned array.
 */
const TMCHAR** ua_parse_dsv(const TMCHAR* line, TMCHAR quote, TMCHAR delim);

/* ua_parse_csv(line)
 *
 * Equivalent to ua_parse_dsv(line, '"', ',');
 */
const TMCHAR** ua_parse_csv(const TMCHAR* line);

/* ua_parse_psv(line)
 *
 * Equivalent to ua_parse_dsv(line, '\0', '|');
 */
const TMCHAR** ua_parse_psv(const TMCHAR* line);

/* ua_free_dsv(data)
 *
 * Frees the vector of TMCHAR strings returned by ua_parse_*
 */
void ua_free_dsv(const TMCHAR** data);

/* UAQuoteStyle enumeration
 *
 * Values:
 *  QUOTE_NEEDED        quote only the fields containing characters that may
 *                      interfere with parsing the resulting line
 *  QUOTE_ALL           quote everything, regarless if it needs it
 *  QUOTE_NONE          disable quoting; equivalent to passing '\0' as a quote
 *                      character to functions accepting it
 *  QUOTE_NONNUMERIC    quote only the fields containing non-numeric
 *                      characters (anything other than '0' ~ '9')
 *
 * When using QUOTE_NEEDED, a field is enclosed in quotes if any of the
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
enum UAQuoteStyle {
    QUOTE_NEEDED = 0,       /* quote only when necessary (see above) */
    QUOTE_ALL,              /* quote everything */
    QUOTE_NONE,             /* disable quoting */
    QUOTE_NONNUMERIC        /* quote things that aren't numbers */
};

/** @region Formatting functions **/

/* ua_format_dsv(data, quotestyle, quotechar, delimchar, escapechar)
 *
 * Format the NULL-terminated array of NULL-terminated strings, @param data,
 * into a single string of delimited fields.
 *
 * @param data      NULL-terminated array of strings to encode
 * @param quoting   quoting style to use (see above)
 * @param quote     quoting character to use, or '\0' for no quoting
 * @param delim     delimiting character to use
 * @param escape    character used to escape special characters, or '\0' to
 *                  use the default mechanism.
 *
 * Returns the formatted string, or NULL on error.
 *
 * Passing '\0' to both @param quote and @param escape disables quoting and
 * character escaping completely.
 *
 * Passing '\0' to just @param escape results in the quoting character being
 * used for escaping.
 *
 * Passing '\0' to just @param quote results in disabled quoting and enabled
 * escaping of special characters.
 *
 */
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
 *
 * If calling this function more than once or for appending data to a file, be
 * sure to provide _TMC("a") as @param mode, as this function opens
 * @param path every time it's called.
 */
int ua_write_dsv(const TMCHAR* path, const TMCHAR* mode,
                 const TMCHAR** data, enum UAQuoteStyle quoting,
                 TMCHAR quote, TMCHAR delim, TMCHAR escape);

/* ua_fwrite_dsv(file, <format-args>)
 *
 * Calls ua_format_dsv with <format-args> and writes the results to
 * @param file.
 *
 * Returns true on success, false on failure.
 */
int ua_fwrite_dsv(UFILE* file,
                  const TMCHAR** data, enum UAQuoteStyle quoting,
                  TMCHAR quote, TMCHAR delim, TMCHAR escape);

/* ua_format_csv(data)
 *
 * Calls ua_format_dsv(data, QUOTE_NEEDED, '"', ',', 0);
 */
const TMCHAR* ua_format_csv(const TMCHAR** data);

/* ua_format_csv(data)
 *
 * Calls ua_write_dsv(path, mode, data, QUOTE_NEEDED, '"', ',', 0);
 */
int ua_write_csv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data);

/* ua_fwrite_csv(file, data)
 *
 * Calls ua_fwrite_dsv(file, data, QUOTE_NEEDED, '"', ',', 0)
 */
int ua_fwrite_csv(UFILE* file, const TMCHAR** data);

/* ua_format_psv(data)
 *
 * Calls ua_format_dsv(data, QUOTE_NONE, 0, '|', 0);
 */
const TMCHAR* ua_format_psv(const TMCHAR** data);

/* ua_write_psv(path, mode, data)
 *
 * Calls ua_write_dsv(path, mode, data, QUOTE_NONE, 0, '|', 0);
 */
int ua_write_psv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data);

/* ua_fwrite_psv(file, data)
 *
 * Calls ua_fwrite_dsv(file, data, QUOTE_NONE, 0, '|', 0)
 */
int ua_fwrite_psv(UFILE* file, const TMCHAR** data);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif
