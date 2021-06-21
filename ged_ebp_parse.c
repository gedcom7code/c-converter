/**
 * The GEDCOM EVent-Base Parser -- the GEDCOM file to Event toolchain.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */

#include <stdlib.h> // for calloc and free
#include <stddef.h> // for ptrdiff_t
#include <ctype.h>  // for isspace

#include "ged_ebp_parse.h"

typedef enum {
    GED_PRE_LEVEL = 0, // between newline and level
    GED_POST_LEVEL, // working on GED_END events before GED_START
    GED_PRE_PAYLOAD, // after space following tag
    GED_POST_TRLR, // all done, no more events 
} GedEventSourceStage;


/**
 * reads from `s` into a malloced string `*dest`, `realloc`ing
 * as needed, stopping at the first character in delims or an 
 * error, whichever comes first. Unlike `getdelim`, it does not
 * include the delimiter in `*dest`, instead returning it.
 * 
 * If `*dest` already has a string, this concatenates to it.
 */
int getUTF8Delim(char **dest, const char *delims, DecodingFileReader *s) {
    size_t cap = 16, len = 0;
    if (*dest) {
        while((*dest)[len]) {
            len += 1;
            if (len >= cap) cap*=2;
        }
    }
    *dest = realloc(*dest, cap);
    int byte;
    for(;;) {
        byte = nextUTF8byte(s);
        if (byte < 0) goto cleanup;
        for(const char *d = delims; *d; d+=1) if (byte == *d) goto cleanup;
        if (len == cap) {
            cap *= 2;
            *dest = realloc(*dest, cap);
        }
        (*dest)[len++] = byte;
    }
    cleanup:
    *dest = realloc(*dest, len+1);
    (*dest)[len] = 0;
    return byte;
}


GedEventSourceState *gedEventSource_create(FILE *in) {
    GedEventSourceState *state = calloc(1, sizeof(GedEventSourceState));
    state->reader = calloc(1, sizeof(DecodingFileReader));
    state->lastLevel = -1;
    int status = decodingFileReader_init(state->reader, in);
    if (status)
        fprintf(stderr, "Error code initializing reader %d\n", status);
    return state;
}

void gedEventSource_free(GedEventSourceState *state) {
    free(state->reader);
    free(state);
}


GedEvent gedEventSource_get(GedEventSourceState *state) {
    GedEvent result;
    result.flags = 0;
    result.data = 0;

#define GED_SE_ERR(msg) do { \
    result.type = GED_ERROR; \
    result.data = msg; \
    state->stage = GED_POST_TRLR; \
    return result; \
} while (0)

    if (state->anchor) {
        result.type = GED_ANCHOR;
        result.data = state->anchor;
        result.flags = GED_OWNS_DATA;
        state->anchor = 0;
        return result;
    }

    switch(state->stage) {
        case GED_PRE_LEVEL: { // post-newline pre-level
            int b = nextCodepoint(state->reader);
            while (isspace(b)) b = nextCodepoint(state->reader);
            if (b == -1) {
                if (state->lastLevel >= 0) {
                    result.type = GED_END;
                    state->lastLevel -= 1;
                    return result;
                }
                result.type = GED_EOF;
                state->stage = GED_POST_TRLR;
                return result;
            } else if (b < -1) GED_SE_ERR("Encountered non-character bytes");
            if (b < '0' || b > '9') {
                GED_SE_ERR("Encountered non-digit when expecting level");
            }
            int level = 0;
            while (b >= '0' && b <= '9') {
                level = (level*10) + (b-'0');
                b = nextCodepoint(state->reader);
                if (b == -1) GED_SE_ERR("File ended mid-line");
                if (b < 0) GED_SE_ERR("Encountered non-character bytes");
            }
            if (!isblank(b)) GED_SE_ERR("Expected space after level");
            state->stage = GED_POST_LEVEL;
            state->inLevel = level;
        } // do not break; fallthrough
        case GED_POST_LEVEL: {
            if (state->lastLevel >= state->inLevel) {
                state->lastLevel -= 1;
                result.type = GED_END;
                return result;
            }
            state->lastLevel = state->inLevel;
            state->stage = GED_PRE_PAYLOAD;

            // read xref:id (if any) and tag
            int b = nextCodepoint(state->reader);
            while (isspace(b)) b = nextCodepoint(state->reader);
            if (b == -1) GED_SE_ERR("File ended mid-line");
            if (b < 0) GED_SE_ERR("Encountered non-character bytes");
            if (b == '@') { // xref:id
                b = getUTF8Delim(&(state->anchor), "@\n\r", state->reader);
                if (b != '@') {
                    free(state->anchor); state->anchor = 0;
                    GED_SE_ERR("unterminated XREF_ID");
                }
                b = nextCodepoint(state->reader);
                while (isspace(b)) b = nextCodepoint(state->reader);
                if (b == -1) GED_SE_ERR("File ended mid-line");
                if (b < 0) GED_SE_ERR("Encountered non-character bytes");
            }
            // tag
            char *ans = malloc(16);
            ans[0] = b; ans[1] = 0;
            b = getUTF8Delim(&ans, " \t\n\r", state->reader);
            if (b == '\n' || b == '\r') state->stage = GED_PRE_LEVEL;
            else if (b == -1) state->stage = GED_POST_TRLR;
            else if (b < 0) GED_SE_ERR("Encountered non-character bytes");
            
            result.type = GED_START;
            result.data = ans;
            result.flags = GED_OWNS_DATA;
            return result;
        } break;
        
        case GED_PRE_PAYLOAD: {
            // messy because of 5.5.1's strange @

            // if @[^#@][^@]*@[\n\r], a pointer
            // if any @@, becomes @
            // if any @#D[^@]*@ ?, a date so make into all-cap with _ instead of ' '
            // if any other @#[^@]*@ ?, remove
            // remaining @ unchanged
            state->stage = GED_PRE_LEVEL;
            // first get the raw characters
            char *payload = 0;
            int b = getUTF8Delim(&payload, "\n\r", state->reader);
            if (b == -1) state->stage = GED_POST_TRLR;
            else if (b < 0) GED_SE_ERR("Encountered non-character bytes");
            // then loop through looking for @ pairs
            ptrdiff_t r=0, w=0, lastAt = -2;
            int esc = 0, ats=0;
            for(;payload[r];r+=1) {
                if (payload[r] != '@' || lastAt < 0) {
                    payload[w] = payload[r];
                    if (payload[r] == '@') { lastAt = w; ats += 1; }
                    if (lastAt == w-1 && payload[w] == '#') esc = 1;
                    w+=1;
                    continue;
                }
                ats += 1;
                if (lastAt == w-1) {
                    lastAt = -2; // un-double
                }
                else if (esc) { // an escape
                    if (payload[lastAt+2] == 'D') { // date
                        // remove "@#D" and skip this "@"; change ' ' to _
                        for(; lastAt+3 < w; lastAt += 1)
                            payload[lastAt] 
                                = (payload[lastAt+3] == ' ') 
                                ? '_' 
                                : payload[lastAt+3];
                        payload[lastAt] = ' ';
                        w -= 2;
                    } else { // pre-5.5 escape; remove
                        w = lastAt;
                    }
                    if (payload[r+1] == ' ') r += 1; // space at end
                    lastAt = -2; // not in paired at anymore
                } else {
                    // previous @ was single and unescaped; ignore it
                    payload[w] = payload[r];
                    lastAt = w;
                    w += 1;
                }
            }
            payload[w] = 0;
            if (ats == 2 && payload[0] == '@' && payload[w-1] == '@') {
                // pointer
                for(r=1; r<w-1; r+=1)
                    payload[r-1] = payload[r];
                payload[r-1] = 0;
                payload = realloc(payload, r);
                result.type = GED_POINTER;
                result.data = payload;
                result.flags = GED_OWNS_DATA;
                return result;
            }
            else if (r != w) payload = realloc(payload, w+1);
            result.type = GED_TEXT;
            result.data = payload;
            result.flags = GED_OWNS_DATA;
            return result;
        }; break;
        
        case GED_POST_TRLR: {
            if (state->inLevel >= 0) {
                state->inLevel -= 1;
                result.type = GED_END;
                return result;
            }
            result.type = GED_EOF;
            return result;
        }; break;
    }
    GED_SE_ERR("coding error: somehow fell through switch");

#undef GED_SE_ERR
}


/// reset internal state so _get will return the first event next
void gedEventSource_rewind(GedEventSourceState *state) {
    decodingFileReader_rewind(state->reader);
    state->stage = GED_PRE_LEVEL;
    state->lastLevel = -1;
    state->inLevel = 0;
}
