/** capitalizes a string in place.
 * Returns 0 is only legal tag characters found,
 * -1 if empty,
 * and the first prohibited byte encountered otherwise
 */
int ged_tagcase_capitalize_legal(char *s) {
    if (!*s) return -1;
    for(;*s;s+=1) {
        if (*s >= '0' && *s <= '9') continue;
        if (*s == '_') continue;
        if (*s >= 'A' && *s <= 'Z') continue;
        if (*s >= 'a' && *s <= 'z') *s &= ~0x20;
        else return *s;
    }
    return 0;
}

/**
 * Force all tags to use upper-case letters.
 * Turn tags with inappropriate characters into GED_ERROR
 */
void ged_tagcase(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    
    if (event->type == GED_START) {
        // WARNING: if !GED_OWNS_DATA, might change someone else's text
        int res = ged_tagcase_capitalize_legal(event->data);
        if (res) {
            ged_destroy_event(event);
            event->type = GED_ERROR;
            event->data = "Encountered illegal character inside tag";
        }
    }
    
    emitter->emit(emitter, *event);
}
