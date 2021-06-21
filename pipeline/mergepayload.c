/**
 * Combines sequences of GED_TEXT and GED_LINEBREAK into single 
 * GED_TEXT with "\n" for the GED_LINEBREAKs
 * 
 * Must happen *after* `ged_unconc` or there will be no 
 * GED_LINEBREAK events to merge
 */

#include <string.h> // strlen, memcpy

struct ged_mergestate {
    char *accumulator;
    size_t used;
};

void ged_merge(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_mergestate *state = (struct ged_mergestate *)rawstate;
    
    if (state->accumulator && (event->type == GED_START || event->type == GED_END)) {
        GedEvent ans;
        ans.data = state->accumulator;
        ans.type = GED_TEXT;
        ans.flags = GED_OWNS_DATA;
        emitter->emit(emitter, ans);
        state->accumulator = 0;
        state->used = 0;
    }
    
    if (event->type == GED_TEXT) {
        if (event->data) {
            size_t more = strlen(event->data);
            if (more > 0) {
                if (!state->accumulator && (event->flags & GED_OWNS_DATA)) {
                    state->accumulator = event->data;
                    state->used = more;
                    event->data = 0;
                    event->flags &= ~GED_OWNS_DATA;
                } else {
                    state->accumulator = realloc(state->accumulator, more + state->used + 1);
                    memcpy(state->accumulator + state->used, event->data, more + 1);
                    state->used += more;
                }
            }
        }
        ged_destroy_event(event);
    } else if (event->type == GED_LINEBREAK) {
        if (!state->accumulator) {
            state->accumulator = calloc(2, sizeof(char));
            state->accumulator[0] = '\n';
            state->used = 1;
        } else {
            state->accumulator = realloc(state->accumulator, state->used+2);
            state->accumulator[state->used++] = '\n';
            state->accumulator[state->used] = '\0';
        }
        ged_destroy_event(event);
    } else {
        emitter->emit(emitter, *event);
    }
}

void *ged_mergestate_maker() { 
    struct ged_mergestate *ans = calloc(1, sizeof(struct ged_mergestate));
    return ans;
}
void ged_mergestate_freer(void *state) { 
    struct ged_mergestate *ans = (struct ged_mergestate *)state;
    if (ans->accumulator) free(ans->accumulator);
    free(state); 
}

#ifndef _GNU_SOURCE
/**
 * Polyfill for strchrnull GNU extension
 */
char *strchrnul(char *s, char c) {
    while(*s && *s != c) s+=1;
    return s;
}
#endif

/**
 * Turns "\n" into GED_LINEBREAK, potentially turning one GED_TEXT into any number of GED_TEXT and GED_LINEBREAK events.
 * 
 * In principle this could be done with no `malloc` calls by having only
 * the last event own the string. However `free` needs the first event
 * to be the one that GED_OWNS_DATA, which means the second and beyond
 * won't work. This could be fixed with additional state (e.g., a second
 * pointer in each GED_EVENT) but this instead `malloc`s each GED_TEXT
 * payload, freeing the original.
 */
void ged_unmerge(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    
    if (event->type == GED_TEXT) {
        GedEvent tmp;
        char *strp = event->data;
        char *nl = strchrnul(strp, '\n');
        if (*nl) {
            for(;;) {
                size_t len = nl-strp;
                if (len > 1) {
                    tmp.flags = GED_OWNS_DATA;
                    tmp.type = GED_TEXT;
                    tmp.data = malloc(len+1);
                    memcpy(tmp.data, strp, len);
                    tmp.data[len] = 0;
                    emitter->emit(emitter, tmp);
                }
                if (*nl) {
                    tmp.flags = 0;
                    tmp.type = GED_LINEBREAK;
                    tmp.data = 0;
                    emitter->emit(emitter, tmp);
                } else break;
                strp = nl+1;
                nl = strchrnul(strp, '\n');
            }
            ged_destroy_event(event);
            return;
        }
    }
    emitter->emit(emitter, *event);
}
