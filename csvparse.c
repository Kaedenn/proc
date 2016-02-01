
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

enum State {
    START_RECORD,
    IN_UNQUOT,
    IN_QUOTE,
    ESCAPE_IN_QUOTE
};

struct Opts {
    CHAR quot;
    CHAR delim;
    CHAR escape;
    bool escape_is_quot;
};

static bool iseol(CHAR c) {
    return c == LIT('\n') || c == LIT('\r') || c == LIT('\0');
}

static bool isws(CHAR c) {
    return c == LIT(' ');
}

static const CHAR* tok_csv(const CHAR* line, const CHAR** out,
                           CHAR quot, CHAR delim) {
    bool usequot = (quot != LIT('\0'));
    const CHAR* end = line;
    int state = START_RECORD;
    CHAR* buffer;
    CHAR* bufpos;
    bool done = false;

    if (iseol(*line)) {
        *out = NULL;
        return NULL;
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
                if (usequot && c == quot) {
                    state = IN_QUOTE;
                } else if (c == delim) {
                    done = true;
                } else if (iseol(c)) {
                    done = true;
                } else if (isws(c)) {
                    /* eat whitespace */
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
                if (usequot && c == quot) {
                    state = ESCAPE_IN_QUOTE;
                } else if (c == LIT('\0')) {
                    done = true;
                } else {
                    *bufpos++ = c;
                }
            } break;
            case ESCAPE_IN_QUOTE: {
                /* encountered a quote in a quoted field, could be either an
                 * escaped dquot or the end of a field */
                if (usequot && c == quot) {
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
        if (c != LIT('\0')) {
            ++end;
        }
    }

    *out = buffer;

    return end;
}


#ifdef TEST

void run_test(const char* input, const char* expected[], char q, char d) {
    const char* r = input;
    const char* out = NULL;
    for (int i = 0; expected[i]; ++i) {
        fprintf(stderr, "Testing %s...\n", r);
        r = tok_csv(r, &out, q, d);
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

#if 0
#ifndef bool
#define bool int
#endif

#ifndef true
#define true 1
#define false 0
#endif

static char* csv_substr(const char* s, size_t start, size_t end) {
    char* result = NULL;
    int i = 0;
    if (end - start <= 0) {
        fprintf(stderr, "error: invalid range [%d,%d] for %s substr",
                start, end, s);
    }
    result = calloc(end - start, sizeof(char));
    for (i = start; i < end; ++i) {
        result[i-start] = s[i];
    }
    return result;
}

static bool isquote(char c) {
    return c == '"';
}

static bool issep(char c) {
    return c == ',';
}

static bool iseol(char c) {
    return c == '\0' || c == '\r' || c == '\n';
}

static const char* parse_eot(const char* piece) {
    bool inquote = false;
    bool infield = false;
    const char* curr = piece;
    while (*curr) {
        if (isquote(*curr) && !infield) {
            inquote = true;
            ++curr;
            infield = true;
        }
        if (*curr && isquote(*curr)) {
            if (isquote(*(curr+1))) {
                // pair quotes: "abcdef\"\"ghi"
                curr += 2;
            } else if (iseol(*(curr+1))) {
            }
        }
        if (*curr && isquote(*curr) && iseol(*(curr+1))) {
    }
}

char** csvparse(const char* line) {
    char** results = NULL;
    // 1) count how many elements to allocate
    int nfields = 1;
    bool inquote = false;
    bool infield = false;
    const char* start = NULL;
    const char* curr = line;
    while (!iseol(*curr)) {
        if (*curr == '"') {
            inquote = !inquote;
        }
        if (*curr == ',' && !inquote) {
            nfields += 1;
        }
        ++curr;
    }
    if (inquote) {
        fprintf(stderr, "parse error: expected \" before EOL in %s", line);
        return NULL;
    }
    // 2) allocate
    results = calloc(nfields, sizeof(char*));
    if (!results) {
        return NULL;
    }
    // 3) store items
    start = curr = line;
    while (!iseol(*curr)) {
        
    }
}

#endif
