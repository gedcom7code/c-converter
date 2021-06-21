/**
 * Change three application-specific IDs to EXID
 * 
 * RFN X -> EXID X + TYPE gedcom551:RFN
 * RIN X -> EXID X + TYPE gedcom551:RIN/<HEAD.SOUR>
 * AFN X -> EXID X + TYPE gedcom551:AFN
 */

struct ged_exidstate {
    char *head_sour;
    char inside;
    char nested_level;
};

#define GED_EXID_HEAD 1
#define GED_EXID_HEAD_SOUR 2
#define GED_EXID_RFN 3
#define GED_EXID_RIN 4
#define GED_EXID_AFN 5
#define GED_EXID_FSFTID 6
#define GED_EXID_APID 7

void ged_exid(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_exidstate *state = (struct ged_exidstate *)rawstate;
    
    if (event->type == GED_START) {
        
        if (state->inside == GED_EXID_HEAD_SOUR) state->inside = GED_EXID_HEAD;
        else if (state->inside > GED_EXID_HEAD) state->inside = 0;
        else if (state->inside) state->nested_level += 1;

        if (state->inside == GED_EXID_HEAD && !strcmp("SOUR",event->data)) {
            state->inside = GED_EXID_HEAD_SOUR;
        } else if (!state->inside) {
            if (!strcmp("RFN", event->data)) {
                state->inside = GED_EXID_RFN;
                changePayloadToConst(event, "EXID");
            }
            else if (!strcmp("RIN", event->data)) {
                state->inside = GED_EXID_RIN;
                changePayloadToConst(event, "EXID");
            }
            else if (!strcmp("AFN", event->data)) {
                state->inside = GED_EXID_AFN;
                changePayloadToConst(event, "EXID");
            }
            else if (!strcmp("_FSFTID", event->data) 
                 || !strcmp("_FID", event->data)
                 || !strcmp("FSFTID", event->data)) {
                state->inside = GED_EXID_FSFTID;
                changePayloadToConst(event, "EXID");
            }
            // add HISTID? Used by FTW5
            else if (!strcmp("_APID", event->data)) {
                state->inside = GED_EXID_APID;
                changePayloadToConst(event, "EXID");
            }
            else if (!strcmp("HEAD", event->data)) state->inside = GED_EXID_HEAD;
        }
    }
    if (event->type == GED_END) {
        if (state->nested_level) state->nested_level -= 1;
        else if (state->inside) state->inside = 0;
    }
    
    if (event->type == GED_TEXT && state->inside > GED_EXID_HEAD) {
        if (state->inside == GED_EXID_HEAD_SOUR) {
            state->head_sour = strdup(event->data);
        }
        else {
            emitter->emit(emitter, *event);
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="TYPE"});
            if (state->inside == GED_EXID_RFN)
                emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="ged551:RFN"});
            if (state->inside == GED_EXID_AFN)
                emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="ged551:AFN"});
            if (state->inside == GED_EXID_FSFTID)
                emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="https://www.familysearch.org/tree/person/"});
            if (state->inside == GED_EXID_APID)
                // not a simple prefix
                // "1,234::567890" becomes "http://search.ancestry.com/cgi-bin/sse.dll?indiv=1&dbid=234&h=567890."
                emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="https://www.ancestry.com/family-tree/"}); 
            if (state->inside == GED_EXID_RIN) {
                if (state->head_sour) {
                    char *uri = calloc(strlen(state->head_sour)+12, 1);
                    sprintf(uri, "ged551:RIN/%s", state->head_sour);
                    emitter->emit(emitter, (GedEvent){GED_TEXT, GED_OWNS_DATA, .data=uri});
                } else {
                    emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="ged551:RIN/unknown"});
                }
            }
            emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
            return;
        }
    }
    emitter->emit(emitter, *event);
}

#undef GED_EXID_HEAD
#undef GED_EXID_HEAD_SOUR
#undef GED_EXID_RFN
#undef GED_EXID_RIN
#undef GED_EXID_AFN


void *ged_exidstate_maker() { 
    struct ged_exidstate *ans = calloc(1, sizeof(struct ged_exidstate));
    return ans;
}
void ged_exidstate_freer(void *rawstate) { 
    struct ged_exidstate *state = (struct ged_exidstate *)rawstate;
    if (state->head_sour) free(state->head_sour);
    free(state); 
}

