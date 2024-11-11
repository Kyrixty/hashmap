#include <stdio.h>
#include "hashmap.h"

int main() {
    hashmap_t *h = hm_new();

    for (size_t i = 0; i < 26; i++) {
        int x = i;
        int *data = &x;
        char *k = malloc(2);
        k[0] = 65 + x % 26;
        k[1] = 0;
        hm_set(h, k, data, sizeof(*data));
        printf("Key: %s, set to: %d\n", k, *data);
    }
    for (size_t i = 0; i < 26; i++) {
        int x = i;
        char *k = malloc(2);
        k[0] = 65 + x % 26;
        k[1] = 0;
        int *data = hm_get(h, k);
        printf("Key: %s, got: %d\n", k, *data);
    }
    return 0;
}