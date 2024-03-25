/*
** stringstore.c
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringstore.h>

/* Increase key value buffer size by 100 when full */
#define KEY_VALUE_BUFFER_SIZE 100

/* Storage of keys and values */
typedef struct {
    char* key;
    char* value;
} KeyValue;

/* Stringstore holding list of keyvalues and the number of words */
struct StringStore {
    KeyValue** words;
    int numWords;
    int bufferSize;
};

StringStore* stringstore_init(void) {
    StringStore* stringStore = malloc(sizeof(StringStore));
    memset(stringStore, 0, sizeof(StringStore)); 

    KeyValue** words = 
	    (KeyValue**)malloc(KEY_VALUE_BUFFER_SIZE * sizeof(KeyValue*));
    stringStore->words = words;
    stringStore->bufferSize = KEY_VALUE_BUFFER_SIZE;
    return stringStore;
}

StringStore* stringstore_free(StringStore* store) {
    // Free inner words
    for (int i = 0; i < store->numWords; i++) {
	free(store->words[i]->key);
	free(store->words[i]->value);
	free(store->words[i]);
    }

    // Free the outter dimension of list
    free(store->words);

    // Free whole stringstore
    free(store);
    return NULL;
}

int stringstore_add(StringStore* store, const char* key, const char* value) {
    char* keyCopy = strdup(key);
    char* valueCopy = strdup(value);
    if (keyCopy == NULL || valueCopy == NULL) {
        return 0;
    }
    
    // If the key exists in the store free the current value and replace it 
    // with the new one
    for (int i = 0; i < store->numWords; i++) {
        if (store->words[i]->key != NULL 
		&& strcmp(store->words[i]->key, key) == 0) {
	    // replace value with new value
	    free(store->words[i]->value);
	    store->words[i]->value = valueCopy;
            return 1;
        }
    }
    
    // If there is a free spot that previously contained a deleted key 
    // (key = NULL), put in the new key and value
    for (int i = 0; i < store->numWords; i++) {
        if (store->words[i]->key == NULL) {
            store->words[i]->key = keyCopy;
            store->words[i]->value = valueCopy;
	    return 1;
	}
    }

    // Add more rows to the words list if the number of words equals the buffer
    // size
    if (store->numWords == store->bufferSize) {
        store->bufferSize = store->bufferSize + KEY_VALUE_BUFFER_SIZE;
        store->words = (KeyValue**)realloc(store->words, 
		sizeof(KeyValue*) * (store->bufferSize));
    }

    // Put the key and value in the words array at last index
    store->words[store->numWords] = (KeyValue*)malloc(sizeof(KeyValue));
    store->words[store->numWords]->key = keyCopy;
    store->words[store->numWords]->value = valueCopy;
    store->numWords++;
    return 1;
}

const char* stringstore_retrieve(StringStore* store, const char* key) {
    for (int i = 0; i < store->numWords; i++) {
        if (store->words[i]->key != NULL 
		&& strcmp(store->words[i]->key, key) == 0) {
	    return (const char*)store->words[i]->value;
	}
    }
    return NULL;
}

int stringstore_delete(StringStore* store, const char* key) {
    for (int i = 0; i < store->numWords; i++) {
	if (store->words[i]->key == NULL) {
	    continue;
	}
        if (strcmp(store->words[i]->key, key) == 0) {
            free(store->words[i]->key);
            free(store->words[i]->value);
            store->words[i]->key = NULL;
            store->words[i]->value = NULL;
	    return 1;
	}
    }
    return 0;
}

