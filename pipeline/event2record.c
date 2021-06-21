/**
 * Given a parsed event-stream, assembles records into tree structure
 * and emits only GED_RECORD-type events, one per record in the input.
 * 
 * Must happen *after* `ged_merge` or some text will be lost.
 */

#include <assert.h>

struct ged_event2record_state {
    // the set of structs that have been opened but not yet closed
    // fixed-size 100 motivated by GEDCOM 5.5.1's levels being [0..99]
    // +1 because GED_END a level-99 will try to zero out level-100
    GedStructure *open[100];
    int depth; // index of first open spot in `open`
};

void ged_event2record(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_event2record_state *state = (struct ged_event2record_state *)rawstate;
    
    switch(event->type) {
        case GED_START: {
            GedStructure *s = calloc(1, sizeof(GedStructure));
            s->tag = *event;
            if (state->open[state->depth]) {
                state->open[state->depth]->sibling = s;
            } else if (state->depth > 0) {
                state->open[state->depth-1]->child = s;
            } else { /* record, nothing to attach it too */ }
            state->open[state->depth++] = s;
        } break;
        case GED_ANCHOR: {
            assert(state->depth > 0);
            state->open[state->depth-1]->anchor = *event;
        } break;
        case GED_TEXT: case GED_POINTER: {
            assert(state->depth > 0);
            assert(state->open[state->depth-1]->payload.type == GED_UNUSED);
            state->open[state->depth-1]->payload = *event;
        } break;
        case GED_END: {
            assert(state->depth > 0);
            state->open[state->depth--] = 0;
            if (state->depth == 0) {
                ged_destroy_event(event);
                event->type = GED_RECORD;
                event->flags = GED_OWNS_DATA;
                event->record = state->open[0];
                emitter->emit(emitter, *event);
                state->open[0] = 0;
            }
        } break;
        case GED_UNUSED: case GED_EOF: case GED_ERROR: case GED_RECORD: {
            emitter->emit(emitter, *event);
        } break;
        case GED_LINEBREAK: assert(0); // handled earlier in pipeline
    }
}

void *ged_event2recordstate_maker() { 
    return calloc(1, sizeof(struct ged_event2record_state));
    
}
void ged_event2recordstate_freer(void *rawstate) { 
    struct ged_event2record_state *state = (struct ged_event2record_state *)rawstate;
    if (state->open[0]) ged_destroy_structure(state->open[0]);
    free(rawstate);
}

