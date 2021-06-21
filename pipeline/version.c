/**
 * Ensure exactly one HEAD.GEDC with one child, VERS 7.0
 */

struct ged_version_state {
    char level;
    char postGedc;
    char cutLevel;
};

void ged_version(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_version_state *state = (struct ged_version_state *)rawstate;
    
    if (event->type == GED_START) {
        state->level += 1;

        if (!state->cutLevel && !strcmp("GEDC",event->data)) {
            state->cutLevel = state->level;
            if (!state->postGedc) {
                state->postGedc = 1;
                emitter->emit(emitter, (GedEvent){GED_START, 0, .data="GEDC"});
                emitter->emit(emitter, (GedEvent){GED_START, 0, .data="VERS"});
                emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="7.0"});
                emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
                emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
            }
        }
    } else if (event->type == GED_END) {
        state->level -= 1;
        if (!state->level && !state->postGedc) {
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="GEDC"});
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="VERS"});
            emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data="7.0"});
            emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
            emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
            state->postGedc = 1;
        }
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

