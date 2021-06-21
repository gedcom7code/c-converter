/**
 * A catch-all for various renames and standardizations, including
 *
 * FORM.TYPE -> FORM.MEDI (payload handled later)
 * 
 * _ASSO -> ASSO
 * _CRE -> CREA
 * _CREAT -> CREA
 * _DATE -> DATE
 * EMAI -> EMAIL
 * _EMAIL -> EMAIL
 * _UID -> UID
 * _INIT -> INIT
 *
 * Defered: _SDATE -> SDATE -- unsure about this one, some _SDATE are "accessed at" dates instead
 * 
 * Should be before `enums`, `rela2role`, and `addschma`
 */

struct ged_rename_state {
    char formLevel;
};

void ged_rename(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    
    struct ged_rename_state *state = (struct ged_rename_state *)rawstate;
    
    if (event->type == GED_START) {
        if (state->formLevel) state->formLevel += 1;
        else if (!strcmp("FORM", event->data)) state->formLevel = 1;
        
        if (state->formLevel == 2 && !strcmp("TYPE", event->data))
            changePayloadToConst(event, "MEDI");
        
        if (!strcmp("_ASSO", event->data))
            changePayloadToConst(event, "ASSO");
        if (!strcmp("_CRE", event->data))
            changePayloadToConst(event, "CREA");
        if (!strcmp("_CREAT", event->data))
            changePayloadToConst(event, "CREA");
        if (!strcmp("_DATE", event->data))
            changePayloadToConst(event, "DATE");
        if (!strcmp("EMAI", event->data))
            changePayloadToConst(event, "EMAIL");
        if (!strcmp("_EMAIL", event->data))
            changePayloadToConst(event, "EMAIL");
        if (!strcmp("_INIT", event->data))
            changePayloadToConst(event, "INIL");
        //if (!strcmp("_SDATE", event->data))
            //changePayloadToConst(event, "SDATE");
        if (!strcmp("_UID", event->data))
            changePayloadToConst(event, "UID");
        
    } else if (event->type == GED_END) {
        if (state->formLevel > 0) state->formLevel -= 1;
    }
    
    emitter->emit(emitter, *event);
}

