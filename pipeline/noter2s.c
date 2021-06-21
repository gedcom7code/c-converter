/**
 * Changes all note records into substructures instead.
 * 
 * Pass 1: collects all note records and stores them for pass 2.
 * This is the only operation in this conversion process that could
 * potentially use O(n) memory.
 * 
 * Pass 2: replaces all "NOTE @ptr@" with a note substructure and 
 * removes all note records.
 * 
 * Pass 1 should be after `event2record`.
 * Both passes should be after `tagcase` and *before* `fixid`.
 * 
 * Additionally, 5.5.1 allowed infinite and circular links because note
 * records could contain sources that could contain pointers to note 
 * records. 7.0 limits this by forcing any note that is a child of a
 * source to be a NOTE_FLAT, not a NOTE_STRUCTURE, where a NOTE_FLAT 
 * has no 5.5.1 substructures (it can have 7.0 MIME and LANG though)
 */

#error "This file represents a change that was rolled back prior to the 7.0 release"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../strtrie.h"

struct ged_noter2s1_state {
    trie records;
    char level;
    char inNote;
    char inNoteRecord;
};

void ged_noter2s1(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_noter2s1_state *state = (struct ged_noter2s1_state *)rawstate;
    
    if (event->type == GED_RECORD
    && !strcmp("NOTE", event->record->tag.data)
    && event->record->anchor.type == GED_ANCHOR) {
        trie_put(&state->records, event->record->anchor.data, event->record);
    } else {
        emitter->emit(emitter, *event);
    }
}

/**
 * Emit a non-GED_OWN_DATA version of a tag
 */
void ged_noter2s_emit_helper(GedEmitterTemplate *emitter, GedEvent e) {
    e.flags &= ~GED_OWNS_DATA;
    emitter->emit(emitter, e);
}
/**
 * like `record2structure`, but with 
 * (a) recursive noter2s converstion, but in a NOTE_FLAT way
 * (b) adjustments to GED_OWNS_DATA to ensure that multiple pointers to same NOTE_RECORD are possible
 * (c) does not free anything
 */
void ged_noter2s_helper(GedEmitterTemplate *emitter, GedStructure *s, struct ged_noter2s1_state *state) {
    GedEvent end;
    end.type = GED_END;
    end.flags = 0;
    end.data = 0;
    while (s) {
        if (!strcmp("NOTE", s->tag.data) && s->payload.type == GED_POINTER) {
            GedStructure *s2 = trie_get(&state->records, s->payload.data);
            if (s2) {
                ged_noter2s_emit_helper(emitter, s2->tag);
                ged_noter2s_emit_helper(emitter, s2->payload);
                emitter->emit(emitter, end);
            }
        } else {
            ged_noter2s_emit_helper(emitter, s->tag);
            if (s->anchor.type != GED_UNUSED)
                ged_noter2s_emit_helper(emitter, s->anchor);
            if (s->payload.type != GED_UNUSED)
                ged_noter2s_emit_helper(emitter, s->payload);
            
            if (s->child) ged_noter2s_helper(emitter, s->child, state);

            emitter->emit(emitter, end);
        }
        
        s = s->sibling;
    }
}


void ged_noter2s2(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_noter2s1_state *state = (struct ged_noter2s1_state *)rawstate;
    
    if (event->type == GED_START) {
        state->level += 1;
        state->inNote = !strcmp("NOTE", event->data);
        if (state->level == 1)
            state->inNoteRecord = state->inNote;
    }
    if (event->type == GED_END) state->level -= 1;

    if (state->inNoteRecord) return; // discard NOTE_RECORD
    
    if (event->type == GED_POINTER && state->inNote) {
        GedStructure *s = trie_get(&state->records, event->data);
        if (!s) { // pointer with no target; emit as text
            event->type = GED_TEXT;
            emitter->emit(emitter, *event);
            return;
        }
        ged_noter2s_emit_helper(emitter, s->payload);
        ged_noter2s_helper(emitter, s->child, state);
 
     } else {
        emitter->emit(emitter, *event);
    }
}


void *ged_noter2s_maker() { 
    return calloc(1, sizeof(struct ged_noter2s1_state));
}

void ged_noter2s_freer(void *rawstate) { 
    struct ged_noter2s1_state *state = (struct ged_noter2s1_state *)rawstate;

    for(size_t i=0; i<2*state->records.length; i+=2) {
        // don't free keys; they are copies also in values and get freed there
        ged_destroy_structure((GedStructure *)state->records.kvpairs[i+1]);
    }
    
    trie_free(&state->records);
    
    free(state); 
}

