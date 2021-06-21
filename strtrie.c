#include <stdlib.h>
#include <stdio.h>
#include "strtrie.h"

struct trie_node_t {
    char c;
    union {
        struct trie_node_t *k; // first child if c != 0
        size_t kvindex; // index into array if c == 0
    };
    struct trie_node_t *s; // next sibling
};

/**
 * searches for string. If found, returns pointer to last trie_node
 * which always has c==0. If not found, returns NULL.
 */
trie_node *trie_get_helper(trie_node *t, const char *key) {
    while(*key && t) {
        while (t && t->c != *key) t = t->s; // linear search
        if (!t) return 0;
        t = t->k;
        key += 1;
    }
    while (t && t->c) t = t->s;
    return t;
}

/**
 * Inserts a string. Returns pointer to last trie_node, which always 
 * has c==0. If already in t, returns same as trie_get; otherwise,
 * allocates nodes using malloc.
 */
trie_node *trie_add_helper(trie_node *t, const char *key) {
    trie_node *old = t;
    while(*key && t) {
        while (t && t->c != *key) { // linear search
            old = t;
            t = t->s;
        }
        if (!t) break;
        t = t->k;
        key += 1;
    }
    if (t && !*key) {
        while (t && t->c) { old=t; t = t->s; }
        if (t) return t; // found it already in tie
    }
    // here old points to last sibling of last row without item
    // add a sibling, then children to the end
    old->s = malloc(sizeof(trie_node));
    t = old->s;
    while(*key) {
        t->c = *key;
        t->s = 0;
        t->k = malloc(sizeof(trie_node));
        key += 1;
        t = t->k;
    }
    t->c = 0;
    t->s = 0;
    t->kvindex = ~(0UL);
    return t;    
}

void *trie_get(trie *t, const char *key) {
    trie_node *leaf = trie_get_helper(t->t, key);
    if (leaf) return t->kvpairs[2*leaf->kvindex+1];
    else return 0;
}
void *trie_put(trie *t, const char *key, void *val) {
    if (!t->t) {
        t->kvpairs = malloc(sizeof(void *)*2);
        t->kvpairs[0] = (void *)key;
        t->kvpairs[1] = val;
        t->length = 1;
        trie_node *tr = malloc(sizeof(trie_node));
        t->t = tr;
        while(*key) {
            tr->c = *key;
            tr->s = 0;
            tr->k = malloc(sizeof(trie_node));
            key += 1;
            tr = tr->k;
        }
        tr->c = 0;
        tr->s = 0;
        tr->kvindex = 0;
        return 0;
    } else {
        trie_node *leaf = trie_add_helper(t->t, key);
        if (leaf->kvindex < ~(0UL)) {
            void *old = t->kvpairs[2*leaf->kvindex + 1];
            t->kvpairs[2*leaf->kvindex + 1] = val;
            return old;
        } else {
            leaf->kvindex = t->length;
            t->length += 1;
            t->kvpairs = realloc(t->kvpairs, t->length*2*sizeof(void *));
            t->kvpairs[2*leaf->kvindex + 0] = (void *)key;
            t->kvpairs[2*leaf->kvindex + 1] = val;
            return 0;
        }
    }
}

void trie_free_helper(trie_node *t) {
    if (!t) return;
    if (t->s) trie_free_helper(t->s);
    if (t->c) trie_free_helper(t->k);
}

void trie_free(trie *t) {
    if (t->t) free(t->kvpairs);
    trie_free_helper(t->t);
    t->t = 0;
}

