
#include <stdlib.h>
#include <stdio.h>

#include "csvparse.h"

/** Notes
 *
 * All records (rows) must contain the same number of entries.
 *
 * Use quotes if embedding newlines in an entry:
 *  Name,Address,Phone
 *  John Doe,"line one
 *  line two
 *  line three",+15555551234 ->
 *      ["Name", "Address", "Phone"],
 *      ["John Doe", "line one\nline two\nline three", "+15555551234"]
 *
 * Use two consecutive quotes to escape a quote: (configurable)
 *  "entry 1", "entry ""number"" two","entry three" ->
 *      ["entry 1", "entry \"number\" two", "entry three"]
 *
 * Quotes in unquoted fields are taken literally:
 *  one,entry "number" two,three -> ["one", "entry \"number\" two", "three"]
 *
 * Rogue quotes (quotes in quoted fields) are also taken literally:
 *  one,"entry "number" two",three -> ["one", "entry \"number\" two", "three"]
 *      NOTE: this may cause a warning
 *
 * Leading and trailing whitespaces are ignored: (configurable?)
 *  one,  two  ,  three -> ["one", "two", "three"]
 *
 * Tabs are not ignored: (configurable?)
 *  one,<TAB>two,<TAB>three -> ["one", "\ttwo", "\tthree"]
 *
 */

enum State {
    START_RECORD,
    IN_UNQUOT,
    IN_QUOTE,
    ESCAPE_IN_QUOTE
};

static bool iseol(TMCHAR c) {
    return c == _TMC('\n') || c == _TMC('\r') || c == _TMC('\0');
}

static bool isws(TMCHAR c) {
    return c == _TMC(' ');
}

static
const TMCHAR* tok_dsv_impl(const TMCHAR* line,  /* parse this line */
                           const TMCHAR** out,  /* store parsed line here */
                           TMCHAR quot,         /* the quot character '"' */
                           TMCHAR delim,        /* the delim character ',' */
                           bool strip_ws,       /* remove extra whitespace */
                           bool tab_is_ws,      /* tabs are whitespace too */
                           TMCHAR escape)       /* escape character '"' */
{
    return NULL;
}

const TMCHAR* tok_dsv(const TMCHAR* line, const TMCHAR** out, TMCHAR quot,
                    TMCHAR delim) {
    const TMCHAR* end = line;
    int state = START_RECORD;
    TMCHAR* buffer;
    TMCHAR* bufpos;
    bool done = false;

    if (iseol(*line)) {
        *out = calloc(1, sizeof(TMCHAR));
        return line;
    }

    buffer = bufpos = calloc(tmstrlen(line)+1, sizeof(TMCHAR));
    if (!buffer) {
        /* bail immediately if there's an allocation problem */
        return NULL;
    }

    while (!done) {
        TMCHAR c = *end;
        switch (state) {
            case START_RECORD: {
                /* initial state */
                if (quot && c == quot) {
                    state = IN_QUOTE;
                } else if (c == delim) {
                    done = true;
                } else if (iseol(c)) {
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
                if (c == delim) {
                    done = true;
                } else if (iseol(c)) {
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
                } else if (c == delim) {
                    done = true;
                } else if (iseol(c)) {
                    done = true;
                } else {
                    /* rogue quote: add it literaly */
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
        /* never traverse past NIL */
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

const TMCHAR** split_dsv(const TMCHAR* line, TMCHAR q, TMCHAR d) {
    const TMCHAR** results = NULL;
    int ridx = 0;
    /* determine initial size */
    int size = 0;
    for (int i = 0; line[i]; ++i) {
        if (line[i] == d) {
            ++size;
        }
    }
    results = calloc(sizeof(const TMCHAR*), size+1);
    if (!results) {
        return NULL;
    }

    const TMCHAR* r = line;
    const TMCHAR* out = NULL;

    while (r && *r) {
        r = tok_dsv(r, &out, q, d);
        results[ridx++] = out;
    }

    return realloc(results, (sizeof(const TMCHAR*)+1)*ridx);
}

const TMCHAR** parse_csv(const TMCHAR* line) {
    return split_dsv(line, _TMC('"'), _TMC(','));
}

const TMCHAR** parse_psv(const TMCHAR* line) {
    return split_dsv(line, _TMC('\0'), _TMC('|'));
}

const TMCHAR* format_csv(const TMCHAR** entries) {
    TMCHAR* buffer = NULL;
    size_t bufsize = 1; /* start with the NULL */
    size_t entry_count = 0;
    for (const TMCHAR** entry = entries; entry; ++entry) {
        entry_count += 1;
        bufsize += tmstrlen(*entry) + 3; /* 1 for NULL, 2 for quotes */
    }

    /* determine what needs to be quoted */
    bool* should_quote = calloc(sizeof(bool), entry_count);
    for (size_t qidx = 0; entries[qidx]; ++qidx) {
        for (size_t i = 0; entries[qidx][i]; ++i) {
            TMCHAR c = entries[qidx][i];
            if (c == _TMC(' ') ||   /* quote anything with spaces */
                c == _TMC('\r') ||  /* quote EOLs */
                c == _TMC('\n') ||  /* quote EOLs */
                c == _TMC(',') ||   /* quote commas */
                (c == _TMC('"') &&  /* quote quotes at the ends */
                 (i == 0 || entries[qidx][i+1] == _TMC('\0')))) {
                should_quote[qidx] = true;
                break;
            }
        }
    }

    /* be pessimistic: assume everything is quotes */
    bufsize *= 2;
    buffer = calloc(sizeof(TMCHAR), bufsize);
    if (!buffer) {
        ua_exit(-1);
    }

    /* write the entries to the buffer */
    size_t i = 0; /* idx to write to */
    for (size_t ei = 0; entries[ei]; ++ei) {
        if (ei != 0) {
            if (i == bufsize) ua_exit(-1);
            buffer[i++] = _TMC(',');
        }
        if (should_quote[ei]) {
            if (i == bufsize) ua_exit(-1);
            buffer[i++] = _TMC('"');
        }
        for (size_t j = 0; entries[ei][j]; ++j) {
            if (entries[ei][j] == _TMC('"')) {
                if (i == bufsize) ua_exit(-1);
                buffer[i++] = _TMC('"');
            }
            if (i == bufsize) ua_exit(-1);
            buffer[i++] = entries[ei][j];
        }
        if (should_quote[ei]) {
            if (i == bufsize) ua_exit(-1);
            buffer[i++] = _TMC('"');
        }
    }

    return realloc(buffer, tmstrlen(buffer) + 1);
}

const TMCHAR* format_psv(const TMCHAR** entries) {
    TMCHAR* buffer = NULL;
    size_t bufsize = 1; /* start with the NULL */
    for (const TMCHAR** entry = entries; *entry; ++entry) {
        bufsize += tmstrlen(*entry) + 1;
    }

    /* allocate buffer with rough estimate of size */
    buffer = calloc(sizeof(TMCHAR), bufsize);
    if (!buffer) {
        ua_exit(-1);
    }

    size_t i = 0;   /* idx to write to */
    for (const TMCHAR** entry = entries; *entry; ++entry) {
        if (*entry != entries[0]) {
            buffer[i++] = _TMC('|');
        }
        for (const TMCHAR* curr = *entry; *curr; ++curr) {
            if (i == bufsize) {
                /* should be unreachable, but here just in case */
                ua_exit(-1);
            }
            /* user should not provide pipes, but ignore them if they do */
            if (*curr == _TMC('|')) {
                continue;
            } else {
                buffer[i++] = *curr;
            }
        }
    }

    /* more error checking which should be never reached */
    if (i == bufsize) {
        ua_exit(-1);
    }

    /* +1 for NULL */
    return realloc(buffer, i+1);
}

#ifdef TEST

#define EPRINTF(...) tmfprintf(&tmBundle, tmstderr, __VA_ARGS__)

void run_test(const TMCHAR* input, const TMCHAR* expected[], TMCHAR q, TMCHAR d) {
    const TMCHAR* r = input;
    const TMCHAR* out = NULL;
    for (int i = 0; expected[i]; ++i) {
        EPRINTF(_TMC("Testing %s...\n"), r);
        r = tok_dsv(r, &out, q, d);
        EPRINTF(_TMC("Obtained \"%s\" (%d)\n"), out, (int)strlen(out));
        EPRINTF(_TMC("Expecting \"%s\"\n"), expected[i]);
        assert(!strcmp(out, expected[i]));
        free((void*)out);
        out = NULL;
    }
    tmfprintf(&tmBundle, tmstderr, _TMC("PASS\n"));
}

void run_test_csv(const TMCHAR* i, const TMCHAR** ex) {
    run_test(i, ex, _TMC('"'), _TMC(','));
}

void run_test_psv(const TMCHAR* i, const TMCHAR** ex) {
    run_test(i, ex, _TMC('\0'), _TMC('|'));
}

void run_test_fmt(const TMCHAR** items /*, const TMCHAR* expected */) {
    const TMCHAR* result = format_psv(items);
    EPRINTF(_TMC("Ended up with %s\n"), result);
    free((void*)result);
    result = format_csv(items);
    EPRINTF(_TMC("Ended up with %s\n"), result);
    free((void*)result);
}

int main(void) {
    const TMCHAR* ans1[] = {_TMC("one"), _TMC("two"), _TMC("three"), NULL};
    run_test_csv(_TMC("one,two,three"), ans1);
    run_test_csv(_TMC("one,\"two\",three"), ans1);
    run_test_csv(_TMC("\"one\",\"two\",\"three\""), ans1);
    run_test_psv(_TMC("one|two|three"), ans1);
    run_test_csv(_TMC("  one  ,  two  ,  three  "), ans1);

    const TMCHAR* ans2[] = {_TMC("one"), _TMC("t\"w\"o"), _TMC("th\"r\"ee"), NULL};
    run_test_csv(_TMC("one,t\"w\"o,\"th\"\"r\"\"ee\""), ans2);
    run_test_psv(_TMC("one|t\"w\"o|th\"r\"ee"), ans2);

    const TMCHAR* ans3[] = {_TMC("one"), _TMC("two\nthree"), _TMC("four"), NULL};
    run_test_csv(_TMC("one,\"two\nthree\",four"), ans3);
    /* NOTE: PSV cannot encode newlines (possible future feature) */

    const TMCHAR* ans4[] = {_TMC("one"), _TMC(""), _TMC("three"), NULL};
    run_test_csv(_TMC("one,\"\",three"), ans4);
    run_test_csv(_TMC("one,,three"), ans4);
    run_test_psv(_TMC("one||three"), ans4);

    const TMCHAR* ans5[] = {_TMC(""), _TMC(""), _TMC("three"), _TMC(""), NULL};
    run_test_csv(_TMC(",\"\",\"three\",\"\""), ans5);
    run_test_csv(_TMC(",,three,"), ans5);
    run_test_psv(_TMC("||three|"), ans5);

    run_test_fmt(ans1);
    run_test_fmt(ans2);
    run_test_fmt(ans3);
    run_test_fmt(ans4);
    run_test_fmt(ans5);

    return 0;
}

#endif

