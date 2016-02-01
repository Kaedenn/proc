
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

static bool iseol(CHAR c) {
    return c == LIT('\n') || c == LIT('\r') || c == LIT('\0');
}

static bool isws(CHAR c) {
    return c == LIT(' ');
}

const CHAR* tok_dsv(const CHAR* line, const CHAR** out, CHAR quot,
                    CHAR delim) {
    const CHAR* end = line;
    int state = START_RECORD;
    CHAR* buffer;
    CHAR* bufpos;
    bool done = false;

    if (iseol(*line)) {
        *out = calloc(1, sizeof(CHAR));
        return line;
    }

    buffer = bufpos = calloc(STRLEN(line)+1, sizeof(CHAR));
    if (!buffer) {
        /* bail immediately if there's an allocation problem */
        return NULL;
    }

    while (!done) {
        CHAR c = *end;
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
                } else if (c == LIT('\0')) {
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
                    *bufpos++ = LIT('"');
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
        if (c != LIT('\0')) {
            ++end;
        }
    }

    /* trim ending whitespace (stopping at empty) */
    while (STRLEN(buffer) > 0 && isws(buffer[STRLEN(buffer)-1])) {
        buffer[STRLEN(buffer)-1] = LIT('\0');
    }

    /* shrink buffer to proper size */
    *out = realloc(buffer, (STRLEN(buffer)+1)*sizeof(CHAR));

    return end;
}

const CHAR** split_dsv(const CHAR* line, CHAR q, CHAR d) {
    const CHAR** results = NULL;
    int ridx = 0;
    /* determine initial size */
    int size = 0;
    for (int i = 0; line[i]; ++i) {
        if (line[i] == d) {
            ++size;
        }
    }
    results = calloc(sizeof(const CHAR*), size+1);
    if (!results) {
        return NULL;
    }

    const CHAR* r = line;
    const CHAR* out = NULL;

    while (r && *r) {
        r = tok_dsv(r, &out, q, d);
        results[ridx++] = out;
    }

    return realloc(results, (sizeof(const CHAR*)+1)*ridx);
}

const CHAR** parse_csv(const CHAR* line) {
    return split_dsv(line, LIT('"'), LIT(','));
}

const CHAR** parse_psv(const CHAR* line) {
    return split_dsv(line, LIT('\0'), LIT('|'));
}

#ifdef TEST

void run_test(const char* input, const char* expected[], char q, char d) {
    const char* r = input;
    const char* out = NULL;
    for (int i = 0; expected[i]; ++i) {
        fprintf(stderr, "Testing %s...\n", r);
        r = tok_dsv(r, &out, q, d);
        fprintf(stderr, "Obtained \"%s\" (%d)\n", out, (int)strlen(out));
        fprintf(stderr, "Expecting \"%s\"\n", expected[i]);
        assert(!strcmp(out, expected[i]));
        free((void*)out);
        out = NULL;
    }
    fprintf(stderr, "PASS\n");
}

void run_test_csv(const char* i, const char** ex) {
    run_test(i, ex, '"', ',');
}

void run_test_psv(const char* i, const char** ex) {
    run_test(i, ex, '\0', '|');
}

int main(void) {
    const char* ans1[] = {"one", "two", "three", NULL};
    run_test_csv("one,two,three", ans1);
    run_test_csv("one,\"two\",three", ans1);
    run_test_csv("\"one\",\"two\",\"three\"", ans1);
    run_test_psv("one|two|three", ans1);
    run_test_csv("  one  ,  two  ,  three  ", ans1);

    const char* ans2[] = {"one", "t\"w\"o", "th\"r\"ee", NULL};
    run_test_csv("one,t\"w\"o,\"th\"\"r\"\"ee\"", ans2);
    run_test_psv("one|t\"w\"o|th\"r\"ee", ans2);

    const char* ans3[] = {"one", "two\nthree", "four", NULL};
    run_test_csv("one,\"two\nthree\",four", ans3);
    /* NOTE: PSV cannot encode newlines (possible future feature) */

    const char* ans4[] = {"one", "", "three", NULL};
    run_test_csv("one,\"\",three", ans4);
    run_test_csv("one,,three", ans4);
    run_test_psv("one||three", ans4);

    const char* ans5[] = {"", "", "three", "", NULL};
    run_test_csv(",\"\",\"three\",\"\"", ans5);
    run_test_csv(",,three,", ans5);
    run_test_psv("||three|", ans5);

    return 0;
}

#endif

