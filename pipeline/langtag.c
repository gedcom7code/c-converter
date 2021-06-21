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

void ged_langtag(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_langtagstate *state = (struct ged_langtagstate *)rawstate;
    
    if (event->type == GED_START)
        state->inLANG = !strcmp("LANG", event->data);
    if (state->inLANG && event->type == GED_TEXT) {
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
    trie_put(&ans->lookup, "Afrikaans", "af");
    trie_put(&ans->lookup, "Albanian", "sq");
    trie_put(&ans->lookup, "Amharic", "am");
    trie_put(&ans->lookup, "Anglo-Saxon", "ang");
    trie_put(&ans->lookup, "Arabic", "ar");
    trie_put(&ans->lookup, "Armenian", "hy");
    trie_put(&ans->lookup, "Assamese", "as");
    trie_put(&ans->lookup, "Belorusian", "be");
    trie_put(&ans->lookup, "Bengali", "bn");
    trie_put(&ans->lookup, "Braj", "bra");
    trie_put(&ans->lookup, "Bulgarian", "bg");
    trie_put(&ans->lookup, "Burmese", "my");
    trie_put(&ans->lookup, "Cantonese", "yue"); // or zh-yue
    trie_put(&ans->lookup, "Catalan", "ca");
    trie_put(&ans->lookup, "Catalan_Spn", "ca-ES"); // not a language?
    trie_put(&ans->lookup, "Church-Slavic", "cu");
    trie_put(&ans->lookup, "Czech", "cs");
    trie_put(&ans->lookup, "Danish", "da");
    trie_put(&ans->lookup, "Dogri", "dgr");
    trie_put(&ans->lookup, "Dutch", "nl");
    trie_put(&ans->lookup, "English", "en");
    trie_put(&ans->lookup, "Esperanto", "eo");
    trie_put(&ans->lookup, "Estonian", "et");
    trie_put(&ans->lookup, "Faroese", "fo");
    trie_put(&ans->lookup, "Finnish", "fi");
    trie_put(&ans->lookup, "French", "fr");
    trie_put(&ans->lookup, "Georgian", "ka");
    trie_put(&ans->lookup, "German", "de");
    trie_put(&ans->lookup, "Greek", "el");
    trie_put(&ans->lookup, "Gujarati", "gu");
    trie_put(&ans->lookup, "Hawaiian", "haw");
    trie_put(&ans->lookup, "Hebrew", "be");
    trie_put(&ans->lookup, "Hindi", "hi");
    trie_put(&ans->lookup, "Hungarian", "hu");
    trie_put(&ans->lookup, "Icelandic", "is");
    trie_put(&ans->lookup, "Indonesian", "id");
    trie_put(&ans->lookup, "Italian", "it");
    trie_put(&ans->lookup, "Japanese", "ja");
    trie_put(&ans->lookup, "Kannada", "kn");
    trie_put(&ans->lookup, "Khmer", "km");
    trie_put(&ans->lookup, "Konkani", "kok");
    trie_put(&ans->lookup, "Korean", "ko");
    trie_put(&ans->lookup, "Lahnda", "lah");
    trie_put(&ans->lookup, "Lao", "lo");
    trie_put(&ans->lookup, "Latvian", "lv");
    trie_put(&ans->lookup, "Lithuanian", "lt");
    trie_put(&ans->lookup, "Macedonian", "mk");
    trie_put(&ans->lookup, "Maithili", "mai");
    trie_put(&ans->lookup, "Malayalam", "ml");
    trie_put(&ans->lookup, "Mandrin", "cmn"); // or zh-cmn
    trie_put(&ans->lookup, "Manipuri", "mni");
    trie_put(&ans->lookup, "Marathi", "mr");
    trie_put(&ans->lookup, "Mewari", "mtr");
    trie_put(&ans->lookup, "Navaho", "nv");
    trie_put(&ans->lookup, "Nepali", "ne");
    trie_put(&ans->lookup, "Norwegian", "no"); // or nb nn, rmg
    trie_put(&ans->lookup, "Oriya", "or");
    trie_put(&ans->lookup, "Pahari", "him"); // or bfz, kfx, mjt, mkb, phr
    trie_put(&ans->lookup, "Pali", "pi");
    trie_put(&ans->lookup, "Panjabi", "pa");
    trie_put(&ans->lookup, "Persian", "fa");
    trie_put(&ans->lookup, "Polish", "pl");
    trie_put(&ans->lookup, "Portuguese", "pt");
    trie_put(&ans->lookup, "Prakrit", "pra");
    trie_put(&ans->lookup, "Pusto", "ps");
    trie_put(&ans->lookup, "Rajasthani", "raj");
    trie_put(&ans->lookup, "Romanian", "ro");
    trie_put(&ans->lookup, "Russian", "ru");
    trie_put(&ans->lookup, "Sanskrit", "sa");
    trie_put(&ans->lookup, "Serb", "sr");
    trie_put(&ans->lookup, "Serbo_Croa", "sh"); // or bs, hr, sr, cnr
    trie_put(&ans->lookup, "Slovak", "sk");
    trie_put(&ans->lookup, "Slovene", "sl");
    trie_put(&ans->lookup, "Spanish", "es");
    trie_put(&ans->lookup, "Swedish", "sv");
    trie_put(&ans->lookup, "Tagalog", "tl");
    trie_put(&ans->lookup, "Tamil", "ta");
    trie_put(&ans->lookup, "Telugu", "te");
    trie_put(&ans->lookup, "Thai", "th");
    trie_put(&ans->lookup, "Tibetan", "bo");
    trie_put(&ans->lookup, "Turkish", "tr");
    trie_put(&ans->lookup, "Ukrainian", "uk");
    trie_put(&ans->lookup, "Urdu", "ur");
    trie_put(&ans->lookup, "Vietnamese", "vi");
    trie_put(&ans->lookup, "Wendic", "wen");
    trie_put(&ans->lookup, "Yiddish", "yi");
    return ans;
}
void ged_langtagstate_freer(void *state) { 
    struct ged_langtagstate *ans = (struct ged_langtagstate *)state;
    trie_free(&ans->lookup);
    free(state); 
}

