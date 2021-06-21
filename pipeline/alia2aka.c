/**
 * Although not part of any previous GEDCOM release, it is fairly common
 * to encounter INDI.ALIA used with a string-valued payload instead of a
 * pointer, with the semantics of an INDI.NAME with TYPE = AKA. This
 * changes that common misapplication into the standard form.
 */

struct ged_alia2aka_state {
    char level;
    char inIndi;
    char inIndiAlia;
};


/**
 * Main entry point, refers most work to a helper so the same code can
 * handle NAME, NAME.FONE, and NAME.ROMN
 */
void ged_alia2aka(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_alia2aka_state *state = (struct ged_alia2aka_state *)rawstate;
    
    if (event->type == GED_START) {
        state->level += 1;
        if (state->level == 1)
            state->inIndi = !strcmp("INDI", event->data);
        state->inIndiAlia = state->inIndi && state->level == 2 && !strcmp("ALIA", event->data);
        if (state->inIndiAlia) { // don't emit this tag yet, might change
            ged_destroy_event(event);
            return; 
        }
    } else if (event->type == GED_END) {
        state->level -= 1;
    } else if (state->inIndiAlia) {
        state->inIndiAlia = 0;
        if (event->type == GED_TEXT) {
            GedEvent tag = {GED_START, 0, .data="NAME"};
            emitter->emit(emitter, tag);
            emitter->emit(emitter, *event);
            GedEvent type = {GED_START, 0, .data="TYPE"};
            emitter->emit(emitter, type);
            GedEvent payload = {GED_TEXT, 0, .data="AKA"};
            emitter->emit(emitter, payload);
            GedEvent end = {GED_END, 0, .data=0};
            emitter->emit(emitter, end);
            return;
        } else {
            GedEvent tag = {GED_START, 0, .data="ALIA"};
            emitter->emit(emitter, tag);
        }
    }
    emitter->emit(emitter, *event);
}
