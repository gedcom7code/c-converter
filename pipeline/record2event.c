/**
 * Given an event stream containing GED_RECORD-type events, emits parse
 * events like GED_START, GED_TEXT, etc to represent the same records.
 */

void ged_record2event_helper(GedEmitterTemplate *emitter, GedStructure *s) {
    GedEvent end;
    end.type = GED_END;
    end.flags = 0;
    end.data = 0;
    while (s) {
        emitter->emit(emitter, s->tag);
        if (s->anchor.type != GED_UNUSED)
            emitter->emit(emitter, s->anchor);
        if (s->payload.type != GED_UNUSED)
            emitter->emit(emitter, s->payload);
        
        if (s->child) ged_record2event_helper(emitter, s->child);

        emitter->emit(emitter, end);

        GedStructure *tmp = s;
        s = s->sibling;
        free(tmp);
    }
}

void ged_record2event(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    if (event->type == GED_RECORD) {
        ged_record2event_helper(emitter, event->record);
    } else {
        emitter->emit(emitter, *event);
    }
}

