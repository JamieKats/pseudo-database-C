#ifndef STRINGSTORE_H
#define STRINGSTORE_H

#include <stdio.h>

//////////
// STRUCTS
//////////

/* Storage of keys and values */
typedef struct {
    char* key;
    char* value;
} KeyValue;

/* Stringstore holding list of keyvalues and the number of words */
typedef struct {
    KeyValue** words;
    int numWords;
    int bufferSize;
} StringStore;

////////////
// FUNCTIONS
////////////
/**
 * Initialise a string store on the heap.
*/
StringStore* stringstore_init(void);

/**
 * Frees the heap memory used by a string store.
*/
StringStore* stringstore_free(StringStore* store);

/**
 * Adds a key value to a stringstore
*/
int stringstore_add(StringStore* store, const char* key, const char* value);

/**
 * Retreives a value from a stringstore.
*/
const char* stringstore_retrieve(StringStore* store, const char* key);

/**
 * Removes a key:value from a stringstore.
*/
int stringstore_delete(StringStore* store, const char* key);

#endif