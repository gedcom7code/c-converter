/**
 * Placeholder (no-op) function
 */
void ged_nop(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    emitter->emit(emitter, *event);
}
void *ged_nostate_maker() { return 0; }
void ged_nostate_freer(void *state) { return; }
