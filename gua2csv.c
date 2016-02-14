
/*****************************************************************************/
/*    Name: gua2csv.c                                                        */
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

#include "gua2csv.h"

/* {{{ REGION: UTIL */

enum {
    CSV_Q = '"',
    CSV_D = ',',
    CSV_E = '\0',

    PSV_Q = '\0',
    PSV_D = '|',
    PSV_E = '\0'
};

enum {
    START_RECORD,
    IN_UNQUOTE,
    IN_QUOTE,
    ESCAPE_IN_QUOTE
};

static int iseol(TMCHAR c) {
    return c == '\n' || c == '\r' || c == '\0';
}

static int isws(TMCHAR c) {
    return c == ' ';
}

static int isnum(TMCHAR c) {
    return c >= '0' && c <= '9';
}

static int isnumeric(const TMCHAR* s) {
    size_t i = 0;
    while (s[i]) {
        if (!isnum(s[i])) return FALSE;
        ++i;
    }
    return TRUE;
}

static size_t veclen(const TMCHAR** vec) {
    size_t i = 0;
    while (vec[i]) { ++i; }
    return i;
}

/* }}} REGION: UTIL */

/* {{{ REGION: UTIL API */

int ua_strcount(const TMCHAR* s, TMCHAR ch) {
    int count = 0;
    int i;
    for (i = 0; s[i]; ++i) {
        if (s[i] == ch) {
            count += 1;
        }
    }
    return count;
}

/* }}} REGION: UTIL API */

/* {{{ REGION: DSV PARSER */

const TMCHAR* ua_dsvtok(const TMCHAR* line, const TMCHAR** out,
                        TMCHAR quot, TMCHAR delim) {
    const TMCHAR* end = line;
    int state = START_RECORD;
    TMCHAR* buffer;
    TMCHAR* bufpos;
    int done = FALSE;

    /* end condition: return an empty string */
    if (iseol(*line)) {
        *out = calloc(1, sizeof(TMCHAR));
        return line;
    }

    /* So, how much memory do we allocate? Well, we're not inserting anything
     * not already present in the input line, so use that as our upper limit */
    buffer = bufpos = calloc(tmstrlen(line)+1, sizeof(TMCHAR));
    if (!buffer) {
        /* bail immediately if there's an allocation problem */
        return NULL;
    }

    while (!done) {
        /* parse character pointed to by `end` */
        TMCHAR c = *end;
        switch (state) {
            case START_RECORD: {
                /* initial state */
                if (quot && c == quot) {
                    state = IN_QUOTE;
                } else if (c == delim || iseol(c)) {
                    done = TRUE;
                } else if (isws(c)) {
                    /* eat initial whitespace */
                } else {
                    *bufpos++ = c;
                    state = IN_UNQUOTE;
                }
            } break;
            case IN_UNQUOTE: {
                /* main state: inside an unquoted field */
                if (c == delim || iseol(c)) {
                    done = TRUE;
                } else {
                    *bufpos++ = c;
                }
            } break;
            case IN_QUOTE: {
                /* main state: inside a quoted field */
                if (quot && c == quot) {
                    state = ESCAPE_IN_QUOTE;
                } else if (c == '\0') {
                    done = TRUE;
                } else {
                    /* no check for \r\n because those are allowed here */
                    *bufpos++ = c;
                }
            } break;
            case ESCAPE_IN_QUOTE: {
                /* encountered a quote in a quoted field, could be either an
                 * escaped dquot or the end of a field */
                if (quot && c == quot) {
                    /* escaped quote, emit one quote */
                    *bufpos++ = c;
                    state = IN_QUOTE;
                } else if (c == delim || iseol(c)) {
                    done = TRUE;
                } else {
                    /* rogue quote: quote found, but following character isn't
                     * special; add character literally */
                    *bufpos++ = CSV_Q;
                    *bufpos++ = c;
                    state = IN_QUOTE;
                }
            } break;
            default:
                /* unreachable, indicates a serious error */
                abort();
                break;
        }
        /* never traverse past a NIL */
        if (c != '\0') {
            ++end;
        }
    }

    /* trim ending whitespace (stopping at empty) */
    while (tmstrlen(buffer) > 0 && isws(buffer[tmstrlen(buffer)-1])) {
        buffer[tmstrlen(buffer)-1] = '\0';
    }

    /* shrink buffer to proper size */
    *out = realloc(buffer, (tmstrlen(buffer)+1)*sizeof(TMCHAR));

    return end;
}

const TMCHAR** ua_parse_dsv(const TMCHAR* line, TMCHAR q, TMCHAR d) {
    /* determine initial size */
    int size = ua_strcount(line, d) + 1; /* +1 as N delims -> N+1 entries */
    const TMCHAR** results = NULL;
    results = calloc(sizeof(const TMCHAR*), size+1); /* +1 for NULL */
    if (!results) {
        return NULL;
    }

    /* prepare for parsing */
    const TMCHAR* r = line;
    const TMCHAR* out = NULL;

    /* parse each element in sequence */
    int ridx = 0;
    while (r && *r) {
        if (ridx > size) {
            /* OOB is highly unlikely, and it is a fatal error */
            tmfprintf(&csvBundle, tmstderr,
                      _TMC("Error parsing {0}: Out of bounds writing to "
                           "index {1,%d}. Aborting"),
                      line, ridx);
            ua_exit(-1);
        }
        /* perform the parsing and store the result */
        r = ua_dsvtok(r, &out, q, d);
        results[ridx++] = out;
    }

    return realloc(results, sizeof(const TMCHAR*)*(ridx+1));
}

const TMCHAR** ua_parse_csv(const TMCHAR* line) {
    return ua_parse_dsv(line, CSV_Q, CSV_D);
}

const TMCHAR** ua_parse_psv(const TMCHAR* line) {
    return ua_parse_dsv(line, PSV_Q, PSV_D);
}

void ua_free_dsv(const TMCHAR** data) {
    const TMCHAR** curr = data;
    while (*curr) {
        free((void*)*curr);
        ++curr;
    }
    free((void*)data);
}

/* }}} REGION: DSV PARSER */

/* {{{ REGION: DSV FORMATTER */

const TMCHAR* ua_format_dsv(const TMCHAR** data,
                            enum UAQuoteStyle quoting,
                            TMCHAR quote,
                            TMCHAR delim,
                            TMCHAR escape) {
    TMCHAR* buffer = NULL;
    size_t buflen = 1; /* 1 for NIL terminator */
    size_t bufpos = 0;
    size_t nitems = 0;
    size_t i, j;

    if (quote == '\0') {
        quoting = QUOTE_NONE;
    } else if (escape == '\0') {
        escape = quote;
    }

    /* 1) calculate amount of space to allocate for buffer */
    for (i = 0; data[i]; ++i) {
        buflen += tmstrlen(data[i]) + 1 /* delim */ + 2 /* quotes */;
        nitems += 1;
    }

    /* worst case: every single character needs escaping */
    buflen *= 2;

    /* short-circuit if nothing to write */
    if (nitems == 0) {
        return calloc(sizeof(TMCHAR), 1);
    }

    /* 2) allocate buffer */
    buffer = calloc(sizeof(TMCHAR), buflen+1);
    if (!buffer) {
        /* let the caller handle errors */
        return NULL;
    }

    /* 3) determine which fields to quote */
    int* should_quote = calloc(sizeof(int), nitems+1);
    switch (quoting) {
        case QUOTE_NEEDED:
            for (i = 0; i < nitems; ++i) {
                const TMCHAR* datum = data[i];
                for (j = 0; datum[j]; ++j) {
                    TMCHAR c = datum[j];
                    if (c == '\r' || c == '\n' || c == delim) {
                        should_quote[i] = TRUE;
                    }
                    if (j == 0 || datum[j+1] == '\0') {
                        if (c == quote || c == ' ') {
                            should_quote[i] = TRUE;
                        }
                    }
                }
            }
            break;
        case QUOTE_ALL:
            for (i = 0; i < nitems; ++i) {
                should_quote[i] = TRUE;
            }
            break;
        case QUOTE_NONE:
            /* noop: calloc already set everything to FALSE */
            break;
        case QUOTE_NONNUMERIC:
            for (i = 0; i < nitems; ++i) {
                should_quote[i] = !isnumeric(data[i]);
            }
            break;
        default:
            tmprintf(&csvBundle,
                     _TMC("{0}:{1,%d}: Error: Invalid quoting style {2,%d}\n"),
                     __FILE__, __LINE__, quoting);
            free((void*)buffer);
            free((void*)should_quote);
            return NULL;
    }

    /* 4) fill the buffer with data */
    for (i = 0; data[i]; ++i) {
        if (i != 0) {
            buffer[bufpos++] = delim;
        }
        if (should_quote[i]) {
            buffer[bufpos++] = quote;
        }
        for (j = 0; data[i][j]; ++j) {
            TMCHAR c = data[i][j];
            if (c == quote || c == delim || c == escape) {
                if (escape) {
                    buffer[bufpos++] = escape;
                }
            }
            buffer[bufpos++] = c;
        }
        if (should_quote[i]) {
            buffer[bufpos++] = quote;
        }
    }

    if (bufpos >= buflen) {
        tmfprintf(&csvBundle, tmstderr, _TMC("Fatal: buffer out of bounds"));
        ua_exit(-1);
    }

    /* 5) clean up and return result */
    free((void*)should_quote);
    return realloc(buffer, sizeof(TMCHAR)*(tmstrlen(buffer)+1));
}

const TMCHAR* ua_format_csv(const TMCHAR** data) {
    return ua_format_dsv(data, QUOTE_NEEDED, CSV_Q, CSV_D, CSV_E);
}

const TMCHAR* ua_format_psv(const TMCHAR** data) {
    return ua_format_dsv(data, QUOTE_NONE, PSV_Q, PSV_D, PSV_E);
}

int ua_write_dsv(const TMCHAR* path, const TMCHAR* mode,
                 const TMCHAR** data, enum UAQuoteStyle quoting,
                 TMCHAR quote, TMCHAR delim, TMCHAR escape) {
    UFILE* f = tmfopen(&csvBundle, path, mode);
    if (!f) {
        return FALSE;
    }

    /* fclose() is technically allowed to modify errno, so save it */
    if (!ua_fwrite_dsv(f, data, quoting, quote, delim, escape)) {
        int save_errno = errno;
        tmfclose(f);
        errno = save_errno;
        return FALSE;
    }

    tmfclose(f);
    return TRUE;
}

int ua_write_csv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data) {
    return ua_write_dsv(path, mode, data, QUOTE_NEEDED, CSV_Q, CSV_D, CSV_E);
}

int ua_write_psv(const TMCHAR* path, const TMCHAR* mode, const TMCHAR** data) {
    return ua_write_dsv(path, mode, data, QUOTE_NONE, PSV_Q, PSV_D, PSV_E);
}

int ua_fwrite_dsv(UFILE* file,
                  const TMCHAR** data, enum UAQuoteStyle quoting,
                  TMCHAR quote, TMCHAR delim, TMCHAR escape) {
    const TMCHAR* buffer = ua_format_dsv(data, quoting, quote, delim, escape);
    if (!buffer) {
        return FALSE;
    }

    tmfprintf(&csvBundle, file, _TMC("{0}\n"), buffer);
    tmfclose(file);
    free(buffer);
    return TRUE;
}

int ua_fwrite_csv(UFILE* file, const TMCHAR** data) {
    return ua_fwrite_dsv(file, data, QUOTE_NEEDED, CSV_Q, CSV_D, CSV_E);
}

int ua_fwrite_psv(UFILE* file, const TMCHAR** data) {
    return ua_fwrite_dsv(file, data, QUOTE_NONE, PSV_Q, PSV_D, PSV_E);
}

/* }}} REGION: DSV FORMATTER */

/* {{{ REGION: DSV SELECT */
#if 0

/* What do we have here, you ask?
 *
 * Well, I'd like to be able to run something akin to:
 *
 * ua_dsv_select(UFILE* file, const TMCHAR* query, const TMCHAR** args,
 *               enum UAQuoteStyle quoting, TMCHAR quotechar,
 *               TMCHAR delimchar, TMCHAR escapechar)
 *
 * and have it be called as
 *
 * const TMCHAR** args = {_TMC("31168931"), NULL};
 * ua_dsv_select(tmstdout, _TMC(
 *      "SELECT spriden_first_name, "
 *             "spriden_mi, "
 *             "spriden_last_name, "
 *             "spriden_id "
 *        "FROM spriden "
 *       "WHERE spriden_id=:id"), args, QUOTE_NEEDED, '"', ',', '\0');
 *
 * which would result in the results of that query being written to stdout as
 * properly formatted CSV.
 *
 * Due to some limitations of PMIG, I can't get this to work quite yet.
 *
 * Due to Pro*C limitations, I doubt this will work with inline SQL without
 * some insane C99 preprocessor hackery, which is turning out to be more
 * trouble than it's worth.
 */

static void strnarrow(const TMCHAR* src, char* dest) {
    while ((*dest++ = (char)*src++)) ;
}

static void strwiden(const char* src, TMCHAR* dest) {
    while ((*dest++ = (TMCHAR)*src++)) ;
}

enum {
    SQLTYPE_CHAR = 1,
    SQLTYPE_NUMERIC = 2,
    SQLTYPE_DECIMAL = 3,
    SQLTYPE_INTEGER = 4,
    SQLTYPE_SMALLINT = 5,
    SQLTYPE_FLOAT = 6,
    SQLTYPE_REAL = 7,
    SQLTYPE_DOUBLE = 8,
    SQLTYPE_DATE = 9,
    SQLTYPE_VARCHAR = 12,

    ORATYPE_VARCHAR2 = 1,   /* char[n] */
    ORATYPE_NUMBER = 2,     /* char[n], n <= 22 */
    ORATYPE_INTEGER = 3,    /* int */
    ORATYPE_FLOAT = 4,      /* float */
    ORATYPE_STRING = 5,     /* char[n+1], char[n]='\0'; */
    ORATYPE_VARNUM = 6,     /* char[n], n <= 22 */
    ORATYPE_DECIMAL = 7,
    ORATYPE_LONG = 8,
    ORATYPE_VARCHAR = 9,
    ORATYPE_ROWID = 11,
    ORATYPE_DATE = 12,
    ORATYPE_VARRAW = 15,
    ORATYPE_SQLT_BFLOAT = 21,
    ORATYPE_SQLT_BDOUBLE = 22,
    ORATYPE_RAW = 23,
    ORATYPE_LONG_RAW = 24,
    ORATYPE_UNSIGNED = 68,
    ORATYPE_DISPLAY = 91,
    ORATYPE_LONG_VARCHAR = 94,
    ORATYPE_LONG_VARRAW = 95,
    ORATYPE_CHAR = 96,
    ORATYPE_CHARF = 96,
    ORATYPE_CHARZ = 97
};

void ua_dsv_select(UFILE* out, const TMCHAR* query, const TMCHAR** inputs,
                   enum UAQuoteStyle quoting, TMCHAR quote, TMCHAR delim,
                   TMCHAR escape)
{
    char* ascquery = calloc(sizeof(char), tmstrlen(query)+1);
    strnarrow(query, ascquery);
    EXEC SQL ALLOCATE DESCRIPTOR 'in'; POSTORA;
    EXEC SQL ALLOCATE DESCRIPTOR 'out'; POSTORA;

    EXEC SQL PREPARE s FROM :ascquery; POSTORA;
    EXEC SQL DESCRIBE INPUT s USING DESCRIPTOR 'in'; POSTORA;
    EXEC SQL DESCRIBE OUTPUT s USING DESCRIPTOR 'out'; POSTORA;

    int ninputs = veclen(inputs);
    int nvars_in = 0;
    int nvars_out = 0;
    EXEC SQL GET DESCRIPTOR 'in' :nvars_in = COUNT; POSTORA;
    EXEC SQL GET DESCRIPTOR 'out' :nvars_out = COUNT; POSTORA;

    if (ninputs < nvars_in) {
        tmfprintf(&csvBundle, tmstdout, _TMC("ERROR!! Not enough inputs!\n"));
        ua_exit(-1);
    } else if (ninputs > nvars_in) {
        tmfprintf(&csvBundle, tmstdout, _TMC("WARNING!! Too many inputs!\n"));
    }

    EXEC SQL DECLARE psv_cur CURSOR FOR s; POSTORA;

    int i = 0;
    for (i = 1; i < nvars_in+1; ++i) {
        const int type = SQLTYPE_CHAR;
        const int length = tmstrlen(inputs[i-1]);
        const TMCHAR* data = inputs[i-1];
        char* ascdata = calloc(sizeof(char), length+1);
        strnarrow(data, ascdata);
        EXEC SQL SET DESCRIPTOR 'in' VALUE :i
            TYPE = :type,
            LENGTH = :length,
            DATA = :ascdata;
        POSTORA;
        free((void*)ascdata);
    }

    EXEC SQL OPEN psv_cur USING DESCRIPTOR 'in'; POSTORA;
    EXEC SQL WHENEVER NOT FOUND DO BREAK; POSTORA;
    while (1) {
        EXEC SQL FETCH psv_cur INTO DESCRIPTOR 'out'; POSTORA;
        if (NO_ROWS_FOUND) break;

        TMCHAR **row = calloc(sizeof(TMCHAR*), nvars_out+1);
        for (i = 1; i < nvars_out+1; ++i) {
            int colsize = 0;
            int coltype = 0;
            EXEC SQL GET DESCRIPTOR 'out' VALUE :i
                :colsize = OCTET_LENGTH,
                :coltype = TYPE;
            POSTORA;
            if (coltype == ORATYPE_DATE) {
                coltype = ORATYPE_VARCHAR2;
                colsize = 12; /* 11 + NULL */
            } else if (coltype == ORATYPE_NUMBER ||
                       coltype == ORATYPE_DECIMAL) {
                coltype = ORATYPE_VARCHAR2;
                colsize = 32; /* upper limit */
            } else if (coltype != ORATYPE_VARCHAR2) {
                coltype = ORATYPE_VARCHAR2;
                colsize = (colsize < 32 ? 32 : colsize) * 4;
            }
            EXEC SQL SET DESCRIPTOR 'out' VALUE :i
                LENGTH = :colsize,
                TYPE = :coltype;
            POSTORA;

            char* value = calloc(sizeof(char), colsize+1);
            EXEC SQL GET DESCRIPTOR 'out' VALUE :i
                :coltype = TYPE,
                :colsize = OCTET_LENGTH,
                :value = DATA;
            POSTORA;
            int j = colsize-1;
            while (j > 0 && value[j] == ' ') {
                value[j--] = '\0';
            }

            row[i-1] = calloc(sizeof(TMCHAR), colsize+1);
            strwiden(value, row[i-1]);
            free((void*)value);
        }

        ua_fwrite_dsv(out, row, quoting, quote, delim, escape);
        ua_free_dsv(row);
    }

    EXEC SQL CLOSE psv_cur; POSTORA;
    EXEC SQL DEALLOCATE DESCRIPTOR 'in'; POSTORA;
    EXEC SQL DEALLOCATE DESCRIPTOR 'out'; POSTORA;
    free(ascquery);
}

#endif /* 0 */
/* }}} REGION: DSV SELECT */


