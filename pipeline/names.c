/*
 * This entire file was halted in an incomplete format, because the
 * change in names was voted down from the 7.0 release
 */


/**
 * Personal names are a mess. Consider the following GEDCOM 5.5.1 
 * 
 * 1 NAME Ⲡⲉⲧⲣⲟⲥ /Ⲡⲉⲧⲣⲟⲥ-Ⲅⲁⲗⲓ/
 * 2 GIVN Ⲡⲉⲧⲣⲟⲥ
 * 2 SPFX Ⲡⲉⲧⲣⲟⲥ
 * 2 SURN Ⲅⲁⲗⲗⲓ
 * 2 ROMN Boutros /Boutros-Ghali/
 * 3 TYPE selected by person
 * 3 GIVN Boutros Boutros
 * 3 SURN Ghali
 * 
 * This is clearly inconsistent in several ways (and incorrect for this
 * politician), but also something a researcher might reasonable create.
 * Because 7 prevents some of these errors, we have some work to do in
 * resolving them during conversion.
 * 
 * This file uses the following algorithm:
 * 
 * 1. The payload, being required and ordered, is used over anything else
 * 2. a SURN is the first match of
 *     1. A SURN substructure with a paylaod that is a substring of the /.../ text
 *     2. The /.../ text with a leading SPFX and/or NPFX and trailing NSFX stripped off, if present
 *     3. The /.../ text
 * 3. Other name parts are kept when
 *     - SPFX: it is a delimited prefix of the /.../ text
 *     - NPFX: it is a delimited prefix of the name
 *     - NSFX: it is a delimited suffix of the name
 *     - GIVN, _RUFNAM: it is a delimited substring of the non-/.../ part of the name
 *     - NICK: it is a delimited substring of the name
 *  
 *     Note, though, that if the same name appears twice in the name text and cannot be distinguished by // and surn-related parts, it is not pinned to any instance.
 * 4. If there is a NICK that is not a delimited substring of the name, it is turned into its own NAME with TYPE NICK
 * 5. If there are name parts not used by the above steps, they are placed in a same-level NOTE
 * 
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


/**
 * Given a 5.5.1-style personal name (NAME, FONE, or ROMN),
 * modifies it into a 7.0-style personal name (without changing tag).
 * If this is inexact, returns a NOTE as well; if exact, returns null.
 */
GedStructure *ged_names_helper(GedStructure *s) {
    // move the entire payload into a PART
    GedStructure *part = calloc(1, sizeof(GedStructure));
    part->payload = s->payload;
    part->tag.data = "PART";
    s->payload.flags &= ~GED_OWNS_DATA;
    s->payload.data = 0;
    #warning "have not yet handled /surname/"
    
    // prep null holder for note
    GedStructure *note = 0;
    
    // iterate through substructures, looking for name parts and translations
    GedStructure **owner = &s->child;
    for(GedStructure *ss = *owner; ss; owner = &ss->sibling, ss = *owner) {
        if (!strcmp("FONE",ss->tag.data)) {
            // change TYPE to LANG
            for(GedStructure *s3 = ss->child; s3; s3 = s3->sibling) {
                if (!strcmp("TYPE", s3->tag.data)) {
                    changePayloadToConst(&(s3->tag), "LANG");
                    if (!strcmp("hangul", s3->payload.data)) {
                        changePayloadToConst(&(s3->payload), "ko-hang");
                    } else if (!strcmp("kana", s3->payload.data)) {
                        changePayloadToConst(&(s3->payload), "jp-hrki");
                    } else {
                        char *payload = calloc(12+strlen(s3->payload.data), 1);
                        sprintf(payload, "x-phonetic-%s", s3->payload.data);
                        changePayloadToDynamic(&(s3->payload), payload);
                    }
                }
            }
            // and handle that it's a name
            GedStructure *tmp = ged_names_helper(ss);
            if (tmp) { tmp->sibling = note; note = tmp; }
            // then change tag
            changePayloadToConst(&(ss->tag), "TRAN");
        } else if (!strcmp("ROMN",ss->tag.data)) {
            // change TYPE to LANG
            for(GedStructure *s3 = ss->child; s3; s3 = s3->sibling) {
                if (!strcmp("TYPE", s3->tag.data)) {
                    changePayloadToConst(&(s3->tag), "LANG");
                    if (!strcmp("pinyin", s3->payload.data)) {
                        changePayloadToConst(&(s3->payload), "und-Latn-pinyin");
                    } else if (!strcmp("romanji", s3->payload.data)) {
                        changePayloadToConst(&(s3->payload), "jp-Latn");
                    } else if (!strcmp("wadegiles", s3->payload.data)) {
                        changePayloadToConst(&(s3->payload), "zh-Latn-wadegile");
                    } else {
                        char *payload = calloc(12+strlen(s3->payload.data), 1);
                        sprintf(payload, "und-Latn-x-%s", s3->payload.data);
                        changePayloadToDynamic(&(s3->payload), payload);
                    }
                }
            }
            // and handle that it's a name
            GedStructure *tmp = ged_names_helper(ss);
            if (tmp) { tmp->sibling = note; note = tmp; }
            // then change tag
            changePayloadToConst(&(ss->tag), "TRAN");
        } else if (!strcmp("GIVN",ss->tag.data)) {
            #warning "GIVN not yet handled"
        } else if (!strcmp("NICK",ss->tag.data)) {
            #warning "NICK not yet handled"
        } else if (!strcmp("NPFX",ss->tag.data)) {
            #warning "NPFX not yet handled"
        } else if (!strcmp("NSFX",ss->tag.data)) {
            #warning "NSFX not yet handled"
        } else if (!strcmp("SPFX",ss->tag.data)) {
            #warning "NSFX not yet handled"
        } else if (!strcmp("SURN",ss->tag.data)) {
            #warning "SURN not yet handled"
        } else if (!strcmp("_RUFNAM",ss->tag.data) || !strcmp("_RUFNAME",ss->tag.data)) {
            #warning "RUFNAME not yet handled"
        }

    }
    
    return note;
}

/**
 * Main entry point, refers most work to a helper so the same code can
 * handle NAME, NAME.FONE, and NAME.ROMN
 */
void ged_names(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    // only interested in NAME structures as direct substructures of INDI
    if (event->type == GED_RECORD
    && !strcmp("INDI", event->record->tag.data)) {
        for(GedStructure *ptr = event->record->child; ptr; ptr = ptr->sibling) {
            if (!strcmp("NAME", ptr->tag.data)) {
                GedStructure *note = ged_names_helper(ptr);
                if (note) {
                    note->sibling = ptr->sibling;
                    ptr->sibling = note;
                }
            }
        }
    }
    emitter->emit(emitter, *event);
}

