/**
 * Ensures that all SOURCE_CITATIONs have a pointer, not text, payload.
 * If a text-payload SOUR substructure is found, emits a new record;
 * in particular, changes
 * 
 *      2 SOUR original text here
 *      3 OBJE ...
 *      3 NOTE ...
 *      3 QUAY ...
 * 
 * into
 * 
 *      0 @id@ SOUR
 *      1 NOTE original text here
 * 
 * and
 * 
 *      2 SOUR @id@
 *      3 OBJE ...
 *      3 NOTE ...
 *      3 QUAY ...
 * 
 * Should be after `event2record` and before `fixid`
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/**
 * Recursively walk the structure, looking for SOUR with text payloads;
 * each one found causes a source record to be emitted and changes it
 * to have a pointer payload instead.
 */
void ged_sours2r_helper(GedStructure *s, GedEmitterTemplate *emitter, long *serial) {
    while(s) {
        if (!strcmp("SOUR", s->tag.data) && s->payload.type == GED_TEXT) {
            
            GedStructure *n = calloc(1, sizeof(GedStructure));
            {
                GedEvent tmp = {GED_START, 0, .data="NOTE"};
                n->tag = tmp;
            }
            n->payload = s->payload;
            
            GedStructure *sr = calloc(1, sizeof(GedStructure));
            sr->child = n;
            {
                GedEvent tmp = {GED_START, 0, .data="SOUR"};
                sr->tag = tmp;
            }
            sr->anchor.type = GED_ANCHOR;
            sr->anchor.flags = GED_OWNS_DATA;
            sr->anchor.data = calloc(32,sizeof(char));
            sprintf(sr->anchor.data, "sours2r id %ld", *serial);
            
            s->payload.type = GED_POINTER;
            s->payload.data = strdup(sr->anchor.data);
            s->payload.flags = GED_OWNS_DATA;
            
            *serial += 1;
            
            {
                GedEvent tmp = {GED_RECORD, GED_OWNS_DATA, .record=sr};
                emitter->emit(emitter, tmp);
            }

            // Create DATA and move any TEXT into it
            GedStructure *data = calloc(1, sizeof(GedStructure));
            data->tag = (GedEvent){GED_START, 0, .data="DATA"};
            GedStructure *tail = 0;
            if (s->child && !strcmp("TEXT", s->child->tag.data)) {
                tail = data->child = s->child;
                data->sibling = s->child->sibling;
                s->child = data;
                tail->sibling = 0;
            }
            GedStructure *end = s->child;
            while(end && end->sibling) {
                if (!strcmp("TEXT", end->sibling->tag.data)) {
                    if (!tail) {
                        tail = data->child = end->sibling;
                        end->sibling = data;
                        data->sibling = tail->sibling;
                        tail->sibling = 0;
                    } else {
                        tail = tail->sibling = end->sibling;
                        end->sibling = end->sibling->sibling;
                        tail->sibling = 0;
                    }
                } else {
                    end = end->sibling;
                }
            }
            if (!tail) free(data);

        }
        ged_sours2r_helper(s->child, emitter, serial);
        s = s->sibling;
    }
}

void ged_sours2r(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    if (event->type == GED_RECORD) {
        if (strcmp("HEAD", event->record->tag.data))
            ged_sours2r_helper(event->record->child, emitter, rawstate);
    }
    emitter->emit(emitter, *event);
}

