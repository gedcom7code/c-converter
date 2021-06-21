/**
 * The GEDCOM EVent-Base Parser -- the Event to GEDCOM file toolchain.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */

#include <stdlib.h> // calloc/free
#include "ged_ebp_emit.h"
#include <assert.h>


GedEventSinkState *gedEventSink_create(FILE *out) {
    GedEventSinkState *state = calloc(1, sizeof(GedEventSinkState));
    state->dest = out;
    return state; 
}

void gedEventSink_free(GedEventSinkState *state) { 
    free(state); 
}

void gedEventSinkFunc(GedEvent evt, GedEventSinkState *state) {

    if (state->last.type == GED_START) {
        // tags have to wait one step to be output
        // because anchor might follow start
        if (evt.type == GED_ANCHOR) 
            fprintf(state->dest, " @%s@", evt.data);
        fprintf(state->dest, " %s", state->last.data);
    }
    
    switch(evt.type) {
        case GED_UNUSED: {
            assert(0); // Must not have unused-type events
        } break;
        case GED_START: {
            fprintf(state->dest, "%s%d",
                (state->last.type ? (
                state->last.type == GED_END ? "" : GED_ENDL
                ) : "\xef\xbb\xbf"), // UTF-8 Byte Order Mark
                state->level
            );
            state->level += 1;
            // tag handled on next event
        } break;
        case GED_END: {
            if (state->last.type != GED_END)
                fprintf(state->dest, GED_ENDL);
            state->level -= 1;
        } break;
        case GED_ANCHOR: {
            // handled before switch
        } break;
        case GED_POINTER: {
            fprintf(state->dest, " @%s@", evt.data);
        } break;
        case GED_TEXT: {
            if (state->last.type == GED_TEXT) {
                fprintf(state->dest, "%s", evt.data);
            } else {
                if (evt.data[0] == '@')
                    fprintf(state->dest, " @%s", evt.data);
                else 
                    fprintf(state->dest, " %s", evt.data);
            }
        } break;
        case GED_LINEBREAK: {
            fprintf(state->dest, "%s%d CONT", GED_ENDL, state->level);
        } break;
        case GED_EOF: {
        } break;
        case GED_ERROR: {
            fprintf(state->dest, "%s0 _PARSE_ERROR %s%s", 
                (state->last.type == GED_END ? "" : GED_ENDL),
                evt.data, GED_ENDL
            );
        } break;
        case GED_RECORD: {
            fprintf(state->dest, "%s0 _PARSE_ERROR <record>%s", 
                (state->last.type == GED_END ? "" : GED_ENDL),
                GED_ENDL
            );
        } break;
    }
    ged_destroy_event(&(state->last));
    state->last = evt;
}
