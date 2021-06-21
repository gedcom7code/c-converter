/**
 * A trie implementation of a map<string, string>.
 * 
 * This is a trie simply because they require less code than a hashmap
 * or self-balancing binary BST. Because iterating through the keys of 
 * a trie requires some nontrivial iterator structures, the trie also
 * incudes a parallel array of key:value pairs. This is space-wasteful
 * and should be replaced if very very large datasets are contemplated.
 * 
 * trie t; t.t = 0;       // no other initialization needed
 * trie_put(&t, "key", "value");
 * trie_get(&t, "key");   // returns "value"
 * trie_put(&t, "key", "value2");
 * trie_get(&t, "key");   // returns "value2"
 * trie_get(&t, "key2");  // returns NULL
 * 
 * // print all entries in first-insertion order
 * for(size_t i=0; i<t.length; i+=1) {
 *      printf("%s: %s\n", t.kvpairs[2*i], t.kvpairs[2*i+1]);
 * }
 * 
 * trie_free(&t);         // frees internal nodes, but not keys or values
 */

#pragma once

typedef struct trie_node_t trie_node;

/**
 * Trie container structure.
 */
typedef struct {
    trie_node *t; // log(n) lookup
    void **kvpairs; // iteration; kvpairs[2*i] is a key, kvpairs[2*i+1] is its value
    size_t length; // number of entries
} trie;

/**
 * If `key` has been previously added using `trie_put`, returns the
 * most recent value that it was given. Otherwise, returns `NULL`.
 */
void *trie_get(trie *t, const char *key);

/**
 * Stores a `key`:`val` mapping. If there was one already, overwrites
 * it with the new mapping; otherwise, creates the mapping.
 * 
 * Returns old value replaced by `val`, or NULL if this is a new key
 */
void *trie_put(trie *t, const char *key, void *val);

/**
 * Free the internal nodes and array used to store the trie. Does not
 * attempt to free the key or value strings themselves.
 */
void trie_free(trie *t);
