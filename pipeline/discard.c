/**
 * Discard tags that have been rendered obsolete:
 * - SUBN and HEAD.SUBN (was specific to now-defunct Temple Ready)
 * - HEAD.CHAR and substructures (now always UTF-8)
 * - HEAD.FILE (redundant and non-synchronized data)
 * - HEAD.GEDC.FORM (LINEAGE_LINKED no longer needed given extensions)
 */

struct ged_discard_state {
    char level;
    char cutLevel;
    char inHeadGedc; // 1 = HEAD, 2 = HEAD.GEDC
};

void ged_discard(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    
    struct ged_discard_state *state = (struct ged_discard_state *)rawstate;
    
    if (event->type == GED_START) {
        state->level += 1;
        
        if (state->level == 1)
            state->inHeadGedc = !strcmp("HEAD",event->data);
        if (state->level == 2 && state->inHeadGedc)
            state->inHeadGedc = 1 + !strcmp("GEDC",event->data);
        
        if (!state->cutLevel)
            if (!strcmp("SUBN",event->data)
            || (state->inHeadGedc == 1 && state->level == 2 && (!strcmp("CHAR",event->data) || !strcmp("FILE",event->data)))
            || (state->inHeadGedc == 2 && state->level == 3 && !strcmp("FORM",event->data))
            ){ 
                state->cutLevel = state->level;
            }
        
    } else if (event->type == GED_END) {
        state->level -= 1;
        if (state->level < state->cutLevel) {
            state->cutLevel = 0;
            ged_destroy_event(event);
            return; // skipping this END
        }
    }
    
    if (!state->cutLevel)
        emitter->emit(emitter, *event);
    else
        ged_destroy_event(event);
}

