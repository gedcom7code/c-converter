#include <string.h>
#include "../geddate.h"


/**
 * Given a DATE, parses it as 5.5.1 and converts to GED 7.0.
 * 
 * Because the main event generator converts `@#DCAL@` to `CAL`,
 * not quite the same as GEDCOM 5.5.1, but very similar.
 * 
 * Because 7.0 has a simpler format than 5.5.1, some DATEs may also
 * emit a PHRASE substructure.
 *
 * Does not validate calendar names or that months match the calendar.
 * Thus, for example, "2 DATE AFTER 12 BEFORE 1" will parse as
 * calendar AFTER, date 12, month BEFORE, year 1.
 * 
 * Dual years like "1567/8" were permitted in 5.5.1, but despite
 * a relatively clear spec were used inconsistently. This implementation
 * will generally allow / after any day, month, and/or year and make
 * the resulting date use the previous part, placing the original 
 * payload in a PHRASE. The only exception is that a bare year (with or
 * without a calendar) with a slash will be transformed into a BET/AND
 * pair; for example "2 DATE JULIAN 1567/1" will become 
 * "2 DATE BET JULIAN 1567 AND JULIAN 1571".
 *
 * Should happen after `ged_tagcase` to reliably identify tags
 * and after `ged_merge` to handle split-payload dates.
 */
void ged_datefix(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    long *isDATE = (long *)state;
    if (event->type == GED_START)
        *isDATE = !strcmp("DATE", event->data) 
            || !strcmp("SDATE", event->data)
            || !strcmp("CHAN", event->data)
            || !strcmp("CREA", event->data)
            ;
    if (*isDATE && event->type == GED_TEXT) {
        GedDateValue *parsed = gedDateParse551(event->data);
        GedEvent evt;
        evt.type = GED_TEXT;
        evt.data = gedDatePayload(parsed);
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
        }
        if (parsed->d2) free(parsed->d2);
        if (parsed->d1) free(parsed->d1);
        if (parsed->freeMe) free(parsed->freeMe);
        free(parsed);
    } else {
        emitter->emit(emitter, *event);
    }
}
