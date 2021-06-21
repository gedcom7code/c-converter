/**
 * Convert from free-text RELA to enumerated ROLE
 */

void ged_rela2role(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    long *state = (long *)rawstate;
    
    if (event->type == GED_START) {
        *state = !strcmp("RELA", event->data);
        if (*state)
            changePayloadToConst(event, "ROLE");
    }
    
    if (event->type == GED_TEXT && *state) {
        char *enumerated = "OTHER";
        if (!strcasecmp("child", event->data)
        || !strcasecmp("kid", event->data)
        || !strcasecmp("chil", event->data))
            enumerated = "CHIL";
        else if (!strcasecmp("husband", event->data)
        || !strcasecmp("hus", event->data))
            enumerated = "HUSB";
        else if (!strcasecmp("wife", event->data))
            enumerated = "WIFE";
        else if (!strcasecmp("mother", event->data)
        || !strcasecmp("moth", event->data))
            enumerated = "MOTH";
        else if (!strcasecmp("father", event->data)
        || !strcasecmp("fath", event->data))
            enumerated = "FATH";
        else if (!strcasecmp("spouse", event->data)
        || !strcasecmp("spou", event->data))
            enumerated = "SPOU";
        else if (!strcasecmp("clergy", event->data)
        || !strcasecmp("priest", event->data)
        || !strcasecmp("pastor", event->data)
        || !strcasecmp("minister", event->data))
            enumerated = "CLERGY";
        else if (!strcasecmp("friend", event->data))
            enumerated = "FRIEND";
        else if (!strcasecmp("godparent", event->data)
        || !strcasecmp("god parent", event->data)
        || !strcasecmp("godmother", event->data)
        || !strcasecmp("god mother", event->data)
        || !strcasecmp("god father", event->data)
        || !strcasecmp("godfather", event->data))
            enumerated = "GODP";
        else if (!strcasecmp("neighbor", event->data))
            enumerated = "NGHBR";
        else if (!strcasecmp("officiator", event->data)
        || !strcasecmp("official", event->data))
            enumerated = "OFFICIATOR";
        else if (!strcasecmp("parent", event->data))
            enumerated = "PARENT";
        else if (!strcasecmp("witness", event->data))
            enumerated = "WITN";
        else
            enumerated = "OTHER";
        
        emitter->emit(emitter, (GedEvent){GED_TEXT, 0, .data=enumerated});
        if (strcasecmp(enumerated, event->data)) {
            emitter->emit(emitter, (GedEvent){GED_START, 0, .data="PHRASE"});
            emitter->emit(emitter, *event);
            emitter->emit(emitter, (GedEvent){GED_END, 0, .data=0});
        } else {
            ged_destroy_event(event);
        }
    } else {
        emitter->emit(emitter, *event);
    }
}

