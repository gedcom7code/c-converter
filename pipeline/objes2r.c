/**
 * Ensures that all MULTIMEDIA_LINKs have a pointer payload, not
 * substructures. 
 * If a text-payload SOUR substructure is found, emits a new record;
 * in particular, changes
 * 
 *      1 OBJE
 *      2 FILE ...
 *      2 TITL ...
 * 
 * into
 * 
 *      0 @id@ OBJE
 *      1 FILE ...
 * 
 * and
 * 
 *      1 OBJE @id@
 *      2 TITL ...
 * 
 * Should be after `event2record` and before both `fixid` and `enums`
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Recursively walk the structure, looking for OBJE with non-pointer 
 * payloads; each one found causes a OBJE record to be emitted and 
 * changes it to have a pointer payload instead.
 */
void ged_objes2r_helper(GedStructure *s, GedEmitterTemplate *emitter, long *serial) {
    while(s) {
        if (!strcmp("OBJE", s->tag.data) && s->payload.type != GED_POINTER) {
            GedStructure *or = calloc(1, sizeof(GedStructure));
            {
                GedEvent tmp = {GED_START, 0, .data="OBJE"};
                or->tag = tmp;
            }
            
            // move all substructures except TITL
            GedStructure **src = &(s->child);
            GedStructure **dst = &(or->child);
            while(*src) {
                if (!strcmp("TITL", (*src)->tag.data)) {
                    src = &((*src)->sibling);
                } else {
                    *dst = *src;
                    *src = (*src)->sibling;
                    dst = &((*dst)->sibling);
                    *dst = 0;
                }
            }
            
            or->anchor.type = GED_ANCHOR;
            or->anchor.flags = GED_OWNS_DATA;
            or->anchor.data = calloc(32,sizeof(char));
            sprintf(or->anchor.data, "objes2r id %ld", *serial);
            
            s->payload.type = GED_POINTER;
            s->payload.data = strdup(or->anchor.data);
            s->payload.flags = GED_OWNS_DATA;
            
            *serial += 1;
            
            {
                GedEvent tmp = {GED_RECORD, GED_OWNS_DATA, .record=or};
                emitter->emit(emitter, tmp);
            }
        }
        ged_objes2r_helper(s->child, emitter, serial);
        s = s->sibling;
    }
}

void ged_objes2r(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    if (event->type == GED_RECORD) {
        ged_objes2r_helper(event->record->child, emitter, rawstate);
    }
    emitter->emit(emitter, *event);
}

