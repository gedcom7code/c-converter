/**
 * removes CONC
 * replaces CONT with GED_LINEBREAK events
 */

#include <string.h> // strcmp

void ged_unconc(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    long *flags = (long *)state;
    if (event->type == GED_START) {
        if (strcmp(event->data, "CONC") == 0) {
            *flags = 1;
            ged_destroy_event(event);
            return;
        }
        if (strcmp(event->data, "CONT") == 0) {
            *flags = 1;
            ged_destroy_event(event);
            GedEvent ans;
            ans.type = GED_LINEBREAK;
            ans.data = 0; ans.flags = 0;
            emitter->emit(emitter, ans);
            return;
        }
    } else if (event->type == GED_END && *flags) {
        *flags = 0;
        ged_destroy_event(event);
        return;
    }
    emitter->emit(emitter, *event);
}

void *ged_longstate_maker() { return calloc(1, sizeof(long)); }
void ged_longstate_freer(void *state) { free(state); }
