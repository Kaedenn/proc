
#include "gor2csv.h"

enum State {
    START_RECORD,
    IN_UNQUOTE,
    IN_QUOTE,
    ESCAPE_IN_QUOTE
};

static bool iseol(TMCHAR c) {
    return c == _TMC('\n') || c == _TMC('\r') || c == _TMC('\0');
}

static bool isws(TMCHAR c) {
    return c == _TMC(' ');
}

static bool isnum(TMCHAR c) {
    return c >= _TMC('0') && c <= _TMC('9');
}

static bool isnumeric(const TMCHAR* s) {
    size_t i = 0;
    while (s[i]) {
        if (!isnum(s[i])) return false;
        ++i;
    }
    return true;
}

int ua_strcount(const TMCHAR* s, TMCHAR ch) {
    int count = 0;
    for (int i = 0; s[i]; ++i) {
        if (s[i] == ch) {
            count += 1;
        }
    }
    return count;
}

const TMCHAR* ua_dsvtok(const TMCHAR* line, const TMCHAR** out,
                        TMCHAR quot, TMCHAR delim) {
    const TMCHAR* end = line;
    int state = START_RECORD;
    TMCHAR* buffer;
    TMCHAR* bufpos;
    bool done = false;

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
                    done = true;
                } else if (isws(c)) {
                    /* eat initial whitespace */
                } else {
                    *bufpos++ = c;
                    state = IN_UNQUOT;
                }
            } break;
            case IN_UNQUOT: {
                /* main state: inside an unquoted field */
                if (c == delim || iseol(c)) {
                    done = true;
                } else {
                    *bufpos++ = c;
                }
            } break;
            case IN_QUOTE: {
                /* main state: inside a quoted field */
                if (quot && c == quot) {
                    state = ESCAPE_IN_QUOTE;
                } else if (c == _TMC('\0')) {
                    done = true;
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
                    done = true;
                } else {
                    /* rogue quote: quote found, but following character isn't
                     * special; add character literally
                     */
                    *bufpos++ = _TMC('"');
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
        if (c != _TMC('\0')) {
            ++end;
        }
    }

    /* trim ending whitespace (stopping at empty) */
    while (tmstrlen(buffer) > 0 && isws(buffer[tmstrlen(buffer)-1])) {
        buffer[tmstrlen(buffer)-1] = _TMC('\0');
    }

    /* shrink buffer to proper size */
    *out = realloc(buffer, (tmstrlen(buffer)+1)*sizeof(TMCHAR));

    return end;
}

const TMCHAR** ua_parse_dsv(const TMCHAR* line, TMCHAR q, TMCHAR d) {
    /* determine initial size */
    int size = ua_strcount(line, d) + 1;    /* 3 delims -> 4 entries */
    results = calloc(sizeof(const TMCHAR*), size+1); /* +1 for NULL */
    if (!results) {
        return NULL;
    }

    const TMCHAR* r = line;
    const TMCHAR* out = NULL;

    const TMCHAR** results = NULL;
    int ridx = 0;
    while (r && *r) {
        if (ridx > size) {
            tmfprintf(&csvBundle, tmstderr, _TMC("Error parsing {0}: "
                                                 "Out of bounds writing to "
                                                 "index {1,%d}. Aborting"),
                      line, ridx);
            ua_exit(-1);
        }
        r = dsvtok(r, &out, q, d);
        results[ridx++] = out;
    }

    return realloc(results, (sizeof(const TMCHAR*)+1)*ridx);
}

const TMCHAR** ua_parse_csv(const TMCHAR* line) {
    return ua_parse_dsv(line, _TMC('"'), _TMC(','));
}

const TMCHAR** ua_parse_psv(const TMCHAR* line) {
    return ua_parse_dsv(line, _TMC('\0'), _TMC('|'));
}

