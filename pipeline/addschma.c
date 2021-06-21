/**
 * A two-pass filter:
 * 
 * pass 1
 *  - reads present schema, stores its mapping for pass 2
 *  - accumulates all used extension tags
 * pass 2
 *  - adds correct schema
 *  - changes tags if remapped
 * 
 * Note that while this is a 5.5.1-to-7 converter, it is designed to
 * handle SCHMA already in the file too.
 * 
 * Should happen after `ged_tagcase` to reliably identify tags.
 * 
 * FIX ME: right now just contains some placeholder URIs. Add correct
 * URIs to ged_addschma_maker as those are determined. Or possibly have
 * the ged_addschma_maker refer to some kind of config file (maybe just
 * a gedcom with a reference schema?)
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../strtrie.h"

enum ged_addschma_flags {
    GED_ADDSCHMA_IN_HEAD = 1,
    GED_ADDSCHMA_IN_SCHMA = 2,
    GED_ADDSCHMA_IN_TAG = 4,
    GED_ADDSCHMA_PASS2 = 8,
    GED_ADDSCHMA_NEED_SCHMA = 16,
};

struct ged_addschma_memory {
    trie known; // constant look-up
    trie decl;  // pass 1 fills with the HEAD.SCHMA, if present
    trie used;  // pass 1 fills with set of _EXTTAG in use
    int level;
    enum ged_addschma_flags  flags;
};

void ged_addschma1(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_addschma_memory *state = (struct ged_addschma_memory *)rawstate;
    
    if (event->type == GED_START) {
        state->level += 1;

        if (state->level == 0 && !strcmp("HEAD",event->data)) {
            state->flags |= GED_ADDSCHMA_IN_HEAD;
        } else if (state->level == 1 && (state->flags & GED_ADDSCHMA_IN_HEAD) && !strcmp("SCHMA",event->data)) {
            state->flags |= GED_ADDSCHMA_IN_SCHMA;
        } else if (state->level == 2 && (state->flags & GED_ADDSCHMA_IN_SCHMA) && !strcmp("TAG",event->data)) {
            state->flags |= GED_ADDSCHMA_IN_TAG;
        }
        
        if (event->data[0] == '_') {
            if (!trie_get(&state->used, event->data))
                trie_put(&state->used, strdup(event->data), 0);
        }
    } else if (event->type == GED_END) {
        if (state->level == 0) state->flags &= ~GED_ADDSCHMA_IN_HEAD;
        if (state->level == 1) state->flags &= ~GED_ADDSCHMA_IN_SCHMA;
        if (state->level == 2) state->flags &= ~GED_ADDSCHMA_IN_TAG;

        state->level -= 1;
    } else if (event->type == GED_TEXT && state->level == 2 && (state->flags & GED_ADDSCHMA_IN_TAG)) {
        char *p = event->data;
        while(isspace(*p)) p+=1;
        char *q = p+1;
        while(*q && !isspace(*q)) q+=1;
        char *tag = strndup(p, q-p);
        while(*q && isspace(*q)) q+=1;
        char *uri = strdup(q);
        const char *old = trie_put(&state->decl, tag, uri);
        if (old) free((void *)old); // should this be an error (same tag twice)?
    }
    
    emitter->emit(emitter, *event);
}

void ged_addschma2(GedEvent *event, GedEmitterTemplate *emitter, void *rawstate) {
    struct ged_addschma_memory *state = (struct ged_addschma_memory *)rawstate;
    
    if (!(state->flags & GED_ADDSCHMA_PASS2)) {
        int need = 0;
        for(size_t i=0; i<2*state->used.length; i+=2)
            if (!trie_get(&state->decl, state->used.kvpairs[i])
            && trie_get(&state->known, state->used.kvpairs[i])) {
                need = 1;
                trie_put(&state->used, state->used.kvpairs[i], 
                    trie_get(&state->known, state->used.kvpairs[i]));
            }
        state->flags |= GED_ADDSCHMA_PASS2;
        if (need) state->flags |= GED_ADDSCHMA_NEED_SCHMA;
    }
    
    if (event->type == GED_START) {
        state->level += 1;

        if (state->level == 0 && !strcmp("HEAD",event->data)) {
            state->flags |= GED_ADDSCHMA_IN_HEAD;
        } else if (state->level == 1 && (state->flags & GED_ADDSCHMA_IN_HEAD) && !strcmp("SCHMA",event->data)) {
            state->flags |= GED_ADDSCHMA_IN_SCHMA;
            state->flags &= ~GED_ADDSCHMA_NEED_SCHMA;
        }
    } else if (event->type == GED_END) {
        if (state->level == 0 && (state->flags & GED_ADDSCHMA_IN_HEAD)) {
            state->flags &= ~GED_ADDSCHMA_IN_HEAD;
            
            if (state->flags & GED_ADDSCHMA_NEED_SCHMA) {
                GedEvent tmp;

                tmp.type = GED_START;
                tmp.data = "SCHMA";
                tmp.flags = 0;
                emitter->emit(emitter, tmp);

                for(size_t i=0; i<2*state->used.length; i+=2)
                    if (state->used.kvpairs[i+1]) {
                        
                        tmp.type = GED_START;
                        tmp.data = "TAG";
                        tmp.flags = 0;
                        emitter->emit(emitter, tmp);

                        tmp.type = GED_TEXT;
                        tmp.data = malloc(2+strlen(state->used.kvpairs[i])+strlen(state->used.kvpairs[i+1]));
                        sprintf(tmp.data, "%s %s", (char *)state->used.kvpairs[i], (char *)state->used.kvpairs[i+1]);
                        tmp.flags = GED_OWNS_DATA;
                        emitter->emit(emitter, tmp);

                        tmp.type = GED_END;
                        tmp.data = 0;
                        tmp.flags = 0;
                        emitter->emit(emitter, tmp);
                    }

                tmp.type = GED_END;
                tmp.data = 0;
                tmp.flags = 0;
                emitter->emit(emitter, tmp);
            }
        } else if (state->level == 1 && (state->flags & GED_ADDSCHMA_IN_SCHMA)) {
            state->flags &= ~GED_ADDSCHMA_IN_SCHMA;

            for(size_t i=0; i<2*state->used.length; i+=2)
                if (state->used.kvpairs[i+1]) {
                    GedEvent tmp;
                    
                    tmp.type = GED_START;
                    tmp.data = "TAG";
                    tmp.flags = 0;
                    emitter->emit(emitter, tmp);

                    tmp.type = GED_TEXT;
                    tmp.data = malloc(2+strlen(state->used.kvpairs[i])+strlen(state->used.kvpairs[i+1]));
                    sprintf(tmp.data, "%s %s", (char *)state->used.kvpairs[i], (char *)state->used.kvpairs[i+1]);
                    tmp.flags = GED_OWNS_DATA;
                    emitter->emit(emitter, tmp);

                    tmp.type = GED_END;
                    tmp.data = 0;
                    tmp.flags = 0;
                    emitter->emit(emitter, tmp);
                }
        }

        state->level -= 1;
    }
    
    emitter->emit(emitter, *event);
}


void *ged_addschma_maker() { 
    struct ged_addschma_memory *ans = calloc(1, sizeof(struct ged_addschma_memory));
    
    ans->level = -1;
    
    trie_put(&ans->known, "_LOC", "http://genealogy.net/GEDCOM#_LOC");
    trie_put(&ans->known, "_GOVTYPE", "http://genealogy.net/GEDCOM#_GOVTYPE");
    trie_put(&ans->known, "_POST", "http://genealogy.net/GEDCOM#_POST");
    trie_put(&ans->known, "_GOV", "http://genealogy.net/GEDCOM#_GOV");
    trie_put(&ans->known, "_MAIDENHEAD", "http://genealogy.net/GEDCOM#_MAIDENHEAD");
    trie_put(&ans->known, "_DMGD", "http://genealogy.net/GEDCOM#_DMGD");
    trie_put(&ans->known, "_AIDN", "http://genealogy.net/GEDCOM#_AIDN");
    // ...
    
    return ans;
}

void ged_addschma_freer(void *rawstate) { 
    struct ged_addschma_memory *state = (struct ged_addschma_memory *)rawstate;
    
    // known: all string literals, so don't free kvpairs
    trie_free(&state->known);
    
    // decl: all from input, so do free kv pair
    for(size_t i=0; i<2*state->decl.length; i+=1)
        free((void *)state->decl.kvpairs[i]);
    trie_free(&state->decl);
    
    // used: keys from input (free); vals from known or decl (do not free)
    for(size_t i=0; i<2*state->used.length; i+=2)
        free((void *)state->used.kvpairs[i]);
    trie_free(&state->used);
    
    free(state); 
}

