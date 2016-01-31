
#include <stdlib.h>
#include <stdio.h>

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

