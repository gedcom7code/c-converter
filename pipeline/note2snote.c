/**
 * Simplest way of making 5.5.1 NOTE conform to 7.0 NOTE/SNOTE:
 * all NOTE_RECORD and all NOTE @ptr@ becomes SNOTE instead.
 * 
 * As a future extension, should be replaced with a heuristic method
 * that assumes NOTE_RECORDS pointed to only once are supposed to be inline.
 */

#include <assert.h>

typedef struct {
    char level;
    char innote;
} ged_note2snote_state;


void ged_note2snote(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    ged_note2snote_state *state = (ged_note2snote_state *)rawstate;
    
    if (event->type == GED_END) state->level -= 1;
    
    if (event->type == GED_START) {
        state->level += 1;
        if (!strcmp("NOTE", event->data)) {
            if (state->level == 1) {
                changePayloadToConst(event, "SNOTE");
                emitter->emit(emitter, *event);
            } else {
                state->innote = 1;
                ged_destroy_event(event);
            }
        } else {
            emitter->emit(emitter, *event);
        }
        if (state->level == 1 && state->innote) {
            state->innote = 0;
        }
    } else if (state->innote) {
        if (event->type == GED_POINTER) {
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="SNOTE"});
            emitter->emit(emitter, *event);
            state->innote = 0;
        } else if (event->type == GED_TEXT || event->type == GED_END) {
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="NOTE"});
            emitter->emit(emitter, *event);
            state->innote = 0;
        } else { // illegal anchor
            ged_destroy_event(event);
        }
    } else {
        emitter->emit(emitter, *event);
    }
}

