#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "hashmap.h"

#define HASHMAP_DEFAULT_BUCKETS 8
#define DEFAULT_BUCKET_CAPACITY 8
#define MAX_LOAD_FACTOR 0.1

typedef struct HashMapItem
{
    void *data;
    char *key;
    size_t size; // size of *item
} hashmap_item_t;

typedef struct HashMapBucket
{
    hashmap_item_t *items;
    size_t length;
    size_t capacity;
} hashmap_bucket_t;

typedef struct HashMap
{
    hashmap_bucket_t *buckets;
    size_t nItems;
    size_t nBuckets;
} hashmap_t;

size_t hash(const char *k, size_t nBuckets)
{
    if (nBuckets == 0)
    {
        return 0;
    }
    size_t sum = 0;
    size_t i = 0;
    while (k[i] != 0)
    {
        sum += k[i];
        i++;
    }
    sum = sum % nBuckets;
    return sum;
}

hashmap_item_t item_new(const char *k, const void *data, size_t size)
{
    void *blob = malloc(size);
    char *newkey = malloc(strlen(k) + 1);
    if (!newkey)
    {
        fprintf(stderr, "item_new :: cannot alloc newkey, oom?\n");
        return (hashmap_item_t){0};
    }
    if (!blob)
    {
        fprintf(stderr, "item_new :: cannot alloc blob, oom?\n");
        return (hashmap_item_t){0};
    }
    strncpy(newkey, k, strlen(k) + 1);
    memcpy(blob, data, size);
    return (hashmap_item_t){
        .data = blob,
        .size = size,
        .key = newkey,
    };
}

void item_destroy(hashmap_item_t item)
{
    if (item.data)
    {
        free(item.data);
    }
    if (item.key)
    {
        free(item.key);
    }
}

hashmap_bucket_t bucket_new(void)
{
    hashmap_bucket_t b = {0};
    b.items = malloc(sizeof(hashmap_item_t) * DEFAULT_BUCKET_CAPACITY);
    if (!b.items)
    {
        fprintf(stderr, "bucket_new :: cannot alloc items, oom?\n");
        return b;
    }
    b.length = 0;
    b.capacity = DEFAULT_BUCKET_CAPACITY;
    return b;
}

int bucket_ensure_capacity(hashmap_bucket_t *b, size_t capacity)
{
    if (!b || capacity <= b->capacity)
    {
        return -1;
    }

    hashmap_item_t *buf = realloc(b->items, capacity * sizeof(hashmap_item_t));
    if (!buf)
    {
        fprintf(stderr, "bucket_resize :: cannot realloc bucket items, oom?\n");
        return 1;
    }
    b->items = buf;
    return 0;
}

/**
 * 1 if size == 0, item.size == 0, b->items is NULL or allocation fails
 * 2 if key already exists (item will still be set)
 */
int bucket_add(hashmap_bucket_t *b, const char *k, const void *data, size_t size)
{
    if (size == 0)
    {
        return 1;
    }
    hashmap_item_t item = item_new(k, data, size);
    if (item.size == 0)
    {
        return 1;
    }
    if (!b->items)
    {
        return 1;
    }
    // check for already set key
    size_t maxCmpLen = strlen(k);
    for (size_t i = 0; i < b->length; i++)
    {
        hashmap_item_t cmpItem = b->items[i];
        if (strncmp(cmpItem.key, k, maxCmpLen) == 0)
        {
            b->items[i] = item;
            return 2;
        }
    }
    if (b->length >= b->capacity)
    {

        if (bucket_ensure_capacity(b, b->capacity * 2) == 1)
        {
            return 1;
        }
    }

    b->items[b->length] = item;
    b->length++;
    return 0;
}

/**
 * Returns NULL if no item is found with key k or data copy fails.
 * Otherwise, returns a copy of the data stored at key k.
 *
 */
void *bucket_get(hashmap_bucket_t *b, const char *k)
{
    size_t maxCmpLen = strlen(k);
    for (size_t i = 0; i < b->length; i++)
    {
        hashmap_item_t item = b->items[i];
        if (!item.key || !item.data)
            continue;
        if (strncmp(item.key, k, maxCmpLen) == 0)
        {
            void *blob = malloc(item.size);
            if (!blob)
            {
                fprintf(stderr, "bucket_get :: cannot alloc new blob copy, oom?\n");
                return NULL;
            }
            memcpy(blob, item.data, item.size);
            return blob;
        }
    }
    return NULL;
}

bool bucket_is_full(hashmap_bucket_t b)
{
    return b.length >= b.capacity;
}

hashmap_t *hm_new(void)
{
    hashmap_t *h = malloc(sizeof(hashmap_t));
    if (!h)
    {
        fprintf(stderr, "hm_new :: cannot alloc h, oom?\n");
        return NULL;
    }
    h->buckets = malloc(sizeof(hashmap_bucket_t) * HASHMAP_DEFAULT_BUCKETS);
    if (!h->buckets)
    {
        fprintf(stderr, "hm_new :: cannot alloc buckets, oom?\n");
        free(h);
        return NULL;
    }
    for (size_t i = 0; i < HASHMAP_DEFAULT_BUCKETS; i++)
    {
        h->buckets[i] = bucket_new();
    }
    h->nItems = 0;
    h->nBuckets = HASHMAP_DEFAULT_BUCKETS;
    return h;
}

/**
 * Returns 1 if allocation fails, -1 if h is NULL, 0 otherwise.
 */
int hm_resize(hashmap_t *h)
{
    if (!h)
    {
        return -1;
    }
    hashmap_bucket_t *old_buckets = h->buckets;
    hashmap_bucket_t *buf = malloc(sizeof(hashmap_bucket_t) * h->nBuckets * 2);
    if (!buf)
    {
        fprintf(stderr, "hm_resize :: cannot resize hashmap, oom?\n");
        return 1;
    }

    h->buckets = buf;
    // Set all new buckets
    for (size_t i = 0; i < h->nBuckets * 2; i++)
    {
        hashmap_bucket_t *new_bucket = h->buckets + i;
        h->buckets[i] = bucket_new();
        new_bucket = h->buckets + i;
        if (!new_bucket->items)
        {
            h->buckets = old_buckets;
            fprintf(stderr, "hm_resize :: cannot create new bucket, oom?\n");
            free(buf);
            return 1;
        }
    }
    for (size_t i = 0; i < h->nBuckets; i++)
    {
        hashmap_bucket_t b = old_buckets[i];
        if (!b.items)
        {
            continue;
        }
        for (size_t j = 0; j < b.length; j++)
        {
            hashmap_item_t item = b.items[j];
            size_t keyHash = hash(item.key, h->nBuckets * 2);
            int status = bucket_add(h->buckets + keyHash, item.key, item.data, item.size);
            if (status != 0)
            {
                h->buckets = old_buckets;
                fprintf(stderr, "hm_resize :: cannot set new elements, oom?\n");
                free(buf);
                return 1;
            }
            // We cannot destroy item here in case hm_set fails in the future
        }
    }

    // hm_set never failed, we can destroy all old items
    // (possible b.c. item_new copies data & key)
    for (size_t i = 0; i < h->nBuckets; i++)
    {
        hashmap_bucket_t b = old_buckets[i];
        if (!b.items)
        {
            continue;
        }
        for (size_t j = 0; j < b.length; j++)
        {
            hashmap_item_t item = b.items[j];
            item_destroy(item);
        }
    }
    free(old_buckets);
    h->nBuckets *= 2;
    return 0;
}

int hm_set(hashmap_t *h, const char *k, const void *data, size_t size)
{
    if (!h || !data || size == 0)
    {
        return -1;
    }
    size_t keyHash = hash(k, h->nBuckets);
    hashmap_bucket_t b = h->buckets[keyHash];
    if (!b.items)
    {
        b = bucket_new();
        if (!b.capacity)
        {
            return 1;
        }
        h->buckets[keyHash] = b;
    }
    if (bucket_is_full(h->buckets[keyHash]) || (h->nItems / h->nBuckets) > MAX_LOAD_FACTOR)
    {
        hm_resize(h);
        keyHash = hash(k, h->nBuckets);
    }
    // printf("keyHash: %zu", keyHash);
    int status = bucket_add(h->buckets + keyHash, k, data, size);
    if (status == 0)
    {
        h->nItems++;
    }
    return status;
}

void *hm_get(const hashmap_t *h, const char *k)
{
    if (!h || !k)
    {
        return NULL;
    }
    size_t keyHash = hash(k, h->nBuckets);
    if (!(h->buckets + keyHash))
    {
        return NULL;
    }
    bucket_get(h->buckets + keyHash, k);
}

bool hm_has(const hashmap_t *h, const char *k)
{
    return hm_get(h, k) != NULL;
}

int hm_remove(hashmap_t *h, const char *k)
{
    if (!h || !k)
    {
        return -1;
    }
    size_t maxCmpLen = strlen(k);
    size_t keyHash = hash(k, h->nBuckets);
    hashmap_bucket_t *b = &h->buckets[keyHash];
    if (!b->items || !b->length || !b->capacity)
        return -1;
    int idx = 0;
    bool match = false;
    for (size_t i = 0; i < b->length; i++)
    {
        hashmap_item_t item = b->items[i];
        if (strncmp(k, item.key, maxCmpLen) == 0)
        {
            match = true;
            continue;
        }

        b->items[idx] = b->items[i];
        idx++;
    }
    if (match)
    {

        b->length--;
        h->nItems--;
    }
    return 0;
}