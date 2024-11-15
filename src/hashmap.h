#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct HashMap hashmap_t;

/**
 * Allocates a new hashmap. Returns NULL if
 * allocation fails.
 */
hashmap_t *hm_new(void);

/**
 * Sets an item entry in the hashmap to data given a key k.
 * Behaviour is undefined if k is not a c-string.
 * Copies memory stored at data into the hashmap.
 * Returns -1 if h or data is NULL or size == 0, 
 * 1 if allocation failed,
 * 0 if operation was successful.
 */
int hm_set(hashmap_t *h, const char *k, const void *data, size_t size);

/**
 * Returns NULL if no item can be found with key k or (h or k) is NULL.
 * 
 * If found, there is no guarantee that the address of
 * the returned pointer will be the same address of the
 * data provided through hm_set
 */
void *hm_get(const hashmap_t *h, const char *k);

/**
 * Returns true if hashmap has an item with corresponding key,
 * false if otherwise.
 */
bool hm_has(const hashmap_t *h, const char *k);

/**
 * Removes an item given a key from the hashmap
 * If not found or (h/k) is NULL, returns -1.
 * Returns 1 if item is successfully deleted.
 */
int hm_remove(hashmap_t *h, const char *k);