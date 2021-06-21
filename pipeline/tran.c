/**
 * FONE and ROMN turn into TRAN, with the following LANGs:
 * 
 * - FONE hangul = ko-hang
 * - FONE kana = jp-hrkt
 * - FONE <other> = x-phonetic-<other>
 * - ROMN pinyin = und-Latn-pinyin
 * - ROMN romanji = jp-Latn
 * - ROMN wadegiles = zh-Latn-wadegile
 * - ROMN <other> = und-Latn-x-<other>
 */


struct ged_tran_state {
    char inWhat; // 1 = FONE, 2 = ROMN, 0 = other
    char depth; // levels past start
    char inType; // 0 = not in FONE.TYPE or ROMN.TYPE; 1 = in
};


/**
 * Main entry point, refers most work to a helper so the same code can
 * handle NAME, NAME.FONE, and NAME.ROMN
 */
void ged_tran(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_tran_state *state = (struct ged_tran_state *)rawstate;
    
    if (event->type == GED_START) {
        if (state->inWhat) state->depth += 1;
        else if (!strcmp("FONE",event->data)) {
            state->inWhat = 1;
            changePayloadToConst(event, "TRAN");
        } else if (!strcmp("ROMN",event->data)) {
            state->inWhat = 2;
            changePayloadToConst(event, "TRAN");
        }
        
        if (state->depth == 1 && !strcmp("TYPE",event->data)) {
            state->inType = 1;
            changePayloadToConst(event, "LANG");
        }
        else state->inType = 0;
    } else if (event->type == GED_END) {
        if (state->inWhat) {
            if (state->depth) state->depth -= 1;
            else state->inWhat = 0;
        }
    } else if (event->type == GED_TEXT && state->inType) {
        if (state->inWhat == 1) { // FONE
            if (!strcmp("hangul", event->data)) {
                changePayloadToConst(event, "ko-hang");
            } else if (!strcmp("kana", event->data)) {
                changePayloadToConst(event, "jp-hrki");
            } else {
                char *payload = calloc(12+strlen(event->data), 1);
                sprintf(payload, "x-phonetic-%s", event->data);
                changePayloadToDynamic(event, payload);
            }
        } else { // ROMN
            if (!strcmp("pinyin", event->data)) {
                changePayloadToConst(event, "und-Latn-pinyin");
            } else if (!strcmp("romanji", event->data)) {
                changePayloadToConst(event, "jp-Latn");
            } else if (!strcmp("wadegiles", event->data)) {
                changePayloadToConst(event, "zh-Latn-wadegile");
            } else {
                char *payload = calloc(12+strlen(event->data), 1);
                sprintf(payload, "und-Latn-x-%s", event->data);
                changePayloadToDynamic(event, payload);
            }
        }
    }
    emitter->emit(emitter, *event);
}
