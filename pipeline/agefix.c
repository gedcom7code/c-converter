#include <string.h>
#include "../gedage.h"


/**
 * Given a AGE, parses it as 5.5.1 and converts to GED 7.0.
 * 
 * The main difference is that 7.0 makes age words into phrases
 * and defines where whitespace may appear. It also allows a count
 * of weeks, but since 5.5.1 didn't that isn't important for the 
 * converter.
 * 
 * If a bare number, y is appended (e.g. "AGE 3" to "AGE 3y").
 * If valid age is followed by other material, the valid part is parsed
 * and the entire payload is copied into a PHRASE.
 *
 * Should happen after `ged_tagcase` to reliably identify tags
 * and after `ged_merge` to handle split-payload ages.
 */
void ged_agefix(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    long *isAGE = (long *)state;
    if (event->type == GED_START)
        *isAGE = !strcmp("AGE", event->data);
    if (*isAGE && event->type == GED_TEXT) {
        GedAge *parsed = gedAgeParse551(event->data);
        GedEvent evt;
        evt.type = GED_TEXT;
        evt.data = gedAgePayload(parsed);
        evt.flags = GED_OWNS_DATA;
        emitter->emit(emitter, evt);
        if (parsed->phrase) {
            evt.type = GED_START;
            evt.data = "PHRASE";
            evt.flags = 0;
            emitter->emit(emitter, evt);
            
            evt.type = GED_TEXT;
            evt.data = parsed->phrase;
            evt.flags = GED_OWNS_DATA;
            emitter->emit(emitter, evt);
            
            evt.type = GED_END;
            evt.data = 0;
            evt.flags = 0;
            emitter->emit(emitter, evt);
        } else {
            ged_destroy_event(event);
        }
        free(parsed);
    } else {
        emitter->emit(emitter, *event);
    }
}
