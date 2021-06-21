#include <string.h>

/**
 * Looks for known enumerated-set tags in 5.5.1 and converst to 7.0 by
 * 
 * - converting to upper case
 * - moving user-defined text into a PHRASE
 * 
 * This code is hard to read because I decided to try to use an existing
 * state instead of fully tracking context and used a lot of strcasecmp
 * instead of making any kind of lookup, but it seems to work OK.
 */

/*
FAMC.ADOP = HUSB|WIFE|BOTH

MEDI = audio|book|card|electronic|fiche|film|magazine|manuscript|map |newspaper|photo|tombstone|video
    * we've added OTHER

PEDI = adopted|birth|foster|sealing 
    * we've added OTHER

RESN = confidential|locked|privacy

ROLE = CHIL|HUSB|WIFE|MOTH|FATH|SPOU|(<ROLE_DESCRIPTOR>)
    * we've added CLERGY|FRIEND|GODP|NGHBR|OFFICIATOR|PARENT|WITN|OTHER

SEX = M|F|U
    * we've added X

FAMC.STAT = challenged|disproven|proven

(Temple ordinance).STAT = BIC|CANCELED|CHILD|COMPLETED|DNS|DNS/CAN|EXCLUDED|INFANT|PRE-1970|STILLBORN|SUBMITTED|UNCLEARED
    * change / and - to _

NAME.TYPE = aka|birth|immigrant|maiden|married|<user defined>
    * we've added NICK|PROFESSIONAL|OTHER

FONE.TYPE -- become TRAN.LANG
ROMN.TYPE -- become TRAN.LANG
 */

/// minimal context tracking
enum {
    GED_ENUM_OTHER,
    GED_ENUM_FAMC, GED_ENUM_FAMC_ADOP, GED_ENUM_FAMC_STAT,
    GED_ENUM_MEDI,
    GED_ENUM_PEDI,
    GED_ENUM_RESN,
    GED_ENUM_ROLE,
    GED_ENUM_SEX,
    GED_ENUM_NAME, GED_ENUM_NAME_TYPE,
    GED_ENUM_TEMPLE,
};
/// simple state tracking; short to ensure it fits in a long
struct ged_enum_state {
    short inside; // one of the above GED_ENUM_*
    char nesting; // for FAMC and NAME, how deep inside we are
    char extnest; // nesting level of top-level extension
};

/**
 * Given a GED_TEXT, emits 4 events:
 * 1. GED_TEXT "OTHER"
 * 2. GED_START "PHRASE"
 * 3. given event
 * 4. GED_END
 */
void ged_enum_other_with_phrase(GedEvent *event, GedEmitterTemplate *emitter) {
    GedEvent tmp;
    
    tmp.type = GED_TEXT;
    tmp.flags = 0;
    tmp.data = "OTHER";
    emitter->emit(emitter, tmp);

    tmp.type = GED_START;
    tmp.flags = 0;
    tmp.data = "PHRASE";
    emitter->emit(emitter, tmp);
    
    // strip (...) if present
    if (event->data[0] == '(') {
        int i;
        for(i=1; event->data[i]; i+=1) {
            event->data[i-1] = event->data[i];
        }
        if (event->data[i-2] == ')') event->data[i-2] = 0;
        else event->data[i-1] = 0;
    }
    emitter->emit(emitter, *event);
    
    tmp.type = GED_END;
    tmp.flags = 0;
    tmp.data = 0;
    emitter->emit(emitter, tmp);
}

/**
 * Given a GED_TEXT, returns a tag-version: upper-case with 
 * non-alphanumerics changed to '_'
 */
void ged_enum_as_tag(GedEvent *event, GedEmitterTemplate *emitter) {
    for(char *c = event->data; *c; c+=1) {
        if (*c >= 'a' && *c <= 'z') *c -= 'a'-'A';
        if ((*c < '0' || *c > '9') && (*c < 'A' || *c > 'Z')) *c = '_';
    }
    emitter->emit(emitter, *event);
}

/**
 * A lot of lookups to handle the known tags of each of nine different
 * enumerated types. Note that FILE.FORM, FONE.TYPE, and ROMN.TYPE are
 * all handled elsewhere, not in this function.
 */
void ged_enums(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    
    struct ged_enum_state *state = (struct ged_enum_state *)rawstate;
    if (event->type == GED_START) {
        state->nesting += 1;
        if (state->extnest || event->data[0] == '_')
        { state->extnest += 1; }
        else if (!strcmp("FAMC", event->data))
        { state->inside = GED_ENUM_FAMC; state->nesting = 0; }
        else if (!strcmp("NAME", event->data))
        { state->inside = GED_ENUM_NAME; state->nesting = 0; }
        else if (!strcmp("MEDI", event->data)) state->inside = GED_ENUM_MEDI;
        else if (!strcmp("PEDI", event->data)) state->inside = GED_ENUM_PEDI;
        else if (!strcmp("RESN", event->data)) state->inside = GED_ENUM_RESN;
        else if (!strcmp("ROLE", event->data)) state->inside = GED_ENUM_ROLE;
        else if (!strcmp("SEX", event->data)) state->inside = GED_ENUM_SEX;
        else if (!strcmp("BAPL", event->data)
              || !strcmp("CONL", event->data)
              || !strcmp("ENDL", event->data)
              || !strcmp("SLGC", event->data)
              || !strcmp("SLGS", event->data)
            ) state->inside = GED_ENUM_TEMPLE;
        else if (state->inside == GED_ENUM_FAMC && state->nesting == 1) {
            if (!strcmp("ADOP", event->data)) state->inside = GED_ENUM_FAMC_ADOP;
            else if (!strcmp("STAT", event->data)) state->inside = GED_ENUM_FAMC_STAT;
        } else if (state->inside == GED_ENUM_NAME && state->nesting == 1) {
            if (!strcmp("TYPE", event->data)) state->inside = GED_ENUM_NAME_TYPE;
        } else {
            if (state->inside == GED_ENUM_FAMC_ADOP || state->inside == GED_ENUM_FAMC_STAT) state->inside = GED_ENUM_FAMC;
            else if (state->inside == GED_ENUM_NAME_TYPE) state->inside = GED_ENUM_NAME;
            else if (state->inside != GED_ENUM_FAMC && state->inside != GED_ENUM_NAME)
                state->inside = GED_ENUM_OTHER;
        }
    }
    if (event->type == GED_END) {
        if (state->extnest > 0) state->extnest -= 1;
        if (state->nesting > 0) state->nesting -= 1;
        if (state->inside == GED_ENUM_FAMC_STAT) state->inside = GED_ENUM_FAMC;
        if (state->inside == GED_ENUM_FAMC_ADOP) state->inside = GED_ENUM_FAMC;
        if (state->inside == GED_ENUM_NAME_TYPE) state->inside = GED_ENUM_NAME;
    }
    
    
    if (event->type == GED_TEXT && (
        state->inside != GED_ENUM_OTHER &&
        state->inside != GED_ENUM_FAMC &&
        state->inside != GED_ENUM_NAME
    )) {
        switch(state->inside) {
            case GED_ENUM_FAMC_ADOP: {
                if (strcasecmp("HUSB", event->data)
                 && strcasecmp("WIFE", event->data)
                 && strcasecmp("BOTH", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_FAMC_STAT: {
                if (strcasecmp("CHALLENGED", event->data)
                 && strcasecmp("DISPROVEN", event->data)
                 && strcasecmp("PROVEN", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_MEDI: {
                if (strcasecmp("AUDIO", event->data)
                 && strcasecmp("BOOK", event->data)
                 && strcasecmp("CARD", event->data)
                 && strcasecmp("ELECTRONIC", event->data)
                 && strcasecmp("FICHE", event->data)
                 && strcasecmp("FILM", event->data)
                 && strcasecmp("MAGAZINE", event->data)
                 && strcasecmp("MANUSCRIPT", event->data)
                 && strcasecmp("MAP ", event->data)
                 && strcasecmp("NEWSPAPER", event->data)
                 && strcasecmp("PHOTO", event->data)
                 && strcasecmp("TOMBSTONE", event->data)
                 && strcasecmp("VIDEO", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_PEDI: {
                if (strcasecmp("ADOPTED", event->data)
                 && strcasecmp("BIRTH", event->data)
                 && strcasecmp("FOSTER", event->data)
                 && strcasecmp("SEALING", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_RESN: {
                if (strcasecmp("CONFIDENTIAL", event->data)
                 && strcasecmp("LOCKED", event->data)
                 && strcasecmp("PRIVACY", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_ROLE: {
                if (strcasecmp("CHIL", event->data)
                 && strcasecmp("HUSB", event->data)
                 && strcasecmp("WIFE", event->data)
                 && strcasecmp("MOTH", event->data)
                 && strcasecmp("FATH", event->data)
                 && strcasecmp("SPOU", event->data)
                 && strcasecmp("CLERGY", event->data)
                 && strcasecmp("FRIEND", event->data)
                 && strcasecmp("GODP", event->data)
                 && strcasecmp("NGHBR", event->data)
                 && strcasecmp("OFFICIATOR", event->data)
                 && strcasecmp("PARENT", event->data)
                 && strcasecmp("WITN", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_SEX: {
                if (strcasecmp("M", event->data)
                 && strcasecmp("F", event->data)
                 && strcasecmp("U", event->data)
                 && strcasecmp("X", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_NAME_TYPE: {
                if (strcasecmp("AKA", event->data)
                 && strcasecmp("BIRTH", event->data)
                 && strcasecmp("IMMIGRANT", event->data)
                 && strcasecmp("MAIDEN", event->data)
                 && strcasecmp("MARRIED", event->data)
                 && strcasecmp("NICK", event->data)
                 && strcasecmp("PROFESSIONAL", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_TEMPLE: {
                if (strcasecmp("BIC", event->data)
                 && strcasecmp("CANCELED", event->data)
                 && strcasecmp("CHILD", event->data)
                 && strcasecmp("COMPLETED", event->data)
                 && strcasecmp("DNS", event->data)
                 && strcasecmp("DNS/CAN", event->data)
                 && strcasecmp("EXCLUDED", event->data)
                 && strcasecmp("INFANT", event->data)
                 && strcasecmp("PRE-1970", event->data)
                 && strcasecmp("STILLBORN", event->data)
                 && strcasecmp("SUBMITTED", event->data)
                 && strcasecmp("UNCLEARED", event->data)
                 && strcasecmp("OTHER", event->data))
                    ged_enum_other_with_phrase(event, emitter);
                else
                    ged_enum_as_tag(event, emitter);
            } break;
            case GED_ENUM_FAMC: case GED_ENUM_NAME: case GED_ENUM_OTHER:
                emitter->emit(emitter, *event);
        }
    } else {
        emitter->emit(emitter, *event);
    }
}

