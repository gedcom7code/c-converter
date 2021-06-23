/**
 * Replaces GEDCOM language names to BCP 47 language tags
 * 
 * Current implementation is picky, requiring exact case and no leading
 * or trailing whitespace. Perhaps it should be changed to fit after
 * enum-normalization to avoid some of those issues?
 *
 *
 * Should happen after `ged_tagcase` to reliably identify tags
 * and after `ged_merge` to handle split-payload ages.
 */

#include "../strtrie.h"
#include <string.h> // for strcmp

struct ged_langtagstate {
    trie lookup;
    int inLANG;
};

static void make_lower_case(char *c) {
    while(*c) {
        if (*c >= 'A' && *c <= 'Z')
            *c ^= ('A'^'a');
        c+=1;
    }
}

void ged_langtag(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_langtagstate *state = (struct ged_langtagstate *)rawstate;
    
    if (event->type == GED_START)
        state->inLANG = !strcmp("LANG", event->data);
    if (state->inLANG && event->type == GED_TEXT) {
        make_lower_case(event->data);
        const char *bcp47 = trie_get(&state->lookup, event->data);
        if (bcp47) {
            ged_destroy_event(event);
            event->type = GED_TEXT;
            event->data = (char *)bcp47; // NOT owned
        } else {
            /* from 
             *  n LANG something
             * to
             *  n LANG und
             *  n+1 PHRASE something
             */
            GedEvent tmp;
            tmp.type = GED_TEXT;
            tmp.data = "und";
            tmp.flags = 0; // NOT owned
            emitter->emit(emitter, tmp);
            tmp.type = GED_START;
            tmp.data = "PHRASE"; // NOT owned
            emitter->emit(emitter, tmp);
            emitter->emit(emitter, *event);
            tmp.type = GED_END;
            tmp.data = 0;
            emitter->emit(emitter, tmp);
            return;
        }
    }
    
    emitter->emit(emitter, *event);
}

void *ged_langtagstate_maker() { 
    struct ged_langtagstate *ans = calloc(1, sizeof(struct ged_langtagstate));
    trie_put(&ans->lookup, "afrikaans", "af");
    trie_put(&ans->lookup, "albanian", "sq");
    trie_put(&ans->lookup, "amharic", "am");
    trie_put(&ans->lookup, "anglo-saxon", "ang");
    trie_put(&ans->lookup, "arabic", "ar");
    trie_put(&ans->lookup, "armenian", "hy");
    trie_put(&ans->lookup, "assamese", "as");
    trie_put(&ans->lookup, "belorusian", "be");
    trie_put(&ans->lookup, "bengali", "bn");
    trie_put(&ans->lookup, "braj", "bra");
    trie_put(&ans->lookup, "bulgarian", "bg");
    trie_put(&ans->lookup, "burmese", "my");
    trie_put(&ans->lookup, "cantonese", "yue"); // or zh-yue
    trie_put(&ans->lookup, "catalan", "ca");
    trie_put(&ans->lookup, "catalan_spn", "ca-es"); // not a language?
    trie_put(&ans->lookup, "church-slavic", "cu");
    trie_put(&ans->lookup, "czech", "cs");
    trie_put(&ans->lookup, "danish", "da");
    trie_put(&ans->lookup, "dogri", "dgr");
    trie_put(&ans->lookup, "dutch", "nl");
    trie_put(&ans->lookup, "english", "en");
    trie_put(&ans->lookup, "esperanto", "eo");
    trie_put(&ans->lookup, "estonian", "et");
    trie_put(&ans->lookup, "faroese", "fo");
    trie_put(&ans->lookup, "finnish", "fi");
    trie_put(&ans->lookup, "french", "fr");
    trie_put(&ans->lookup, "georgian", "ka");
    trie_put(&ans->lookup, "german", "de");
    trie_put(&ans->lookup, "greek", "el");
    trie_put(&ans->lookup, "gujarati", "gu");
    trie_put(&ans->lookup, "hawaiian", "haw");
    trie_put(&ans->lookup, "hebrew", "be");
    trie_put(&ans->lookup, "hindi", "hi");
    trie_put(&ans->lookup, "hungarian", "hu");
    trie_put(&ans->lookup, "icelandic", "is");
    trie_put(&ans->lookup, "indonesian", "id");
    trie_put(&ans->lookup, "italian", "it");
    trie_put(&ans->lookup, "japanese", "ja");
    trie_put(&ans->lookup, "kannada", "kn");
    trie_put(&ans->lookup, "khmer", "km");
    trie_put(&ans->lookup, "konkani", "kok");
    trie_put(&ans->lookup, "korean", "ko");
    trie_put(&ans->lookup, "lahnda", "lah");
    trie_put(&ans->lookup, "lao", "lo");
    trie_put(&ans->lookup, "latvian", "lv");
    trie_put(&ans->lookup, "lithuanian", "lt");
    trie_put(&ans->lookup, "macedonian", "mk");
    trie_put(&ans->lookup, "maithili", "mai");
    trie_put(&ans->lookup, "malayalam", "ml");
    trie_put(&ans->lookup, "mandrin", "cmn"); // or zh-cmn
    trie_put(&ans->lookup, "manipuri", "mni");
    trie_put(&ans->lookup, "marathi", "mr");
    trie_put(&ans->lookup, "mewari", "mtr");
    trie_put(&ans->lookup, "navaho", "nv");
    trie_put(&ans->lookup, "nepali", "ne");
    trie_put(&ans->lookup, "norwegian", "no"); // or nb nn, rmg
    trie_put(&ans->lookup, "oriya", "or");
    trie_put(&ans->lookup, "pahari", "him"); // or bfz, kfx, mjt, mkb, phr
    trie_put(&ans->lookup, "pali", "pi");
    trie_put(&ans->lookup, "panjabi", "pa");
    trie_put(&ans->lookup, "persian", "fa");
    trie_put(&ans->lookup, "polish", "pl");
    trie_put(&ans->lookup, "portuguese", "pt");
    trie_put(&ans->lookup, "prakrit", "pra");
    trie_put(&ans->lookup, "pusto", "ps");
    trie_put(&ans->lookup, "rajasthani", "raj");
    trie_put(&ans->lookup, "romanian", "ro");
    trie_put(&ans->lookup, "russian", "ru");
    trie_put(&ans->lookup, "sanskrit", "sa");
    trie_put(&ans->lookup, "serb", "sr");
    trie_put(&ans->lookup, "serbo_croa", "sh"); // or bs, hr, sr, cnr
    trie_put(&ans->lookup, "slovak", "sk");
    trie_put(&ans->lookup, "slovene", "sl");
    trie_put(&ans->lookup, "spanish", "es");
    trie_put(&ans->lookup, "swedish", "sv");
    trie_put(&ans->lookup, "tagalog", "tl");
    trie_put(&ans->lookup, "tamil", "ta");
    trie_put(&ans->lookup, "telugu", "te");
    trie_put(&ans->lookup, "thai", "th");
    trie_put(&ans->lookup, "tibetan", "bo");
    trie_put(&ans->lookup, "turkish", "tr");
    trie_put(&ans->lookup, "ukrainian", "uk");
    trie_put(&ans->lookup, "urdu", "ur");
    trie_put(&ans->lookup, "vietnamese", "vi");
    trie_put(&ans->lookup, "wendic", "wen");
    trie_put(&ans->lookup, "yiddish", "yi");
    return ans;
}
void ged_langtagstate_freer(void *state) { 
    struct ged_langtagstate *ans = (struct ged_langtagstate *)state;
    trie_free(&ans->lookup);
    free(state); 
}

