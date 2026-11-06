#ifndef HIERARCHY_H
#define HIERARCHY_H

#include <stdint.h>
#include "config.h"
#include "cache.h"

/*
 * Stores caches in levels, formatted as L[index+1].
 * Ex: L[0] relates to L1 cache.
 */
typedef struct {
    Cache** levels;
    size_t num_levels;
} CacheHierarchy;

/*
 * Dynamically alocates cache hierarchy structure.
 * 
 * Accepts path to hierarchy config file.
 */
CacheHierarchy* create_cache_hierarchy(const char* input);

/*
 * Deallocates cache hierarchy structure and associated caches.
 */
void free_cache_hierarchy(CacheHierarchy* hierarchy);

/*
 * Creates a request to a cache hierarchy.
 * 
 * Requests provide the referenced memory address, its size in bytes, and
 * Read/Write. An access that spans several blocks is split into one request
 * per block at each level. A miss fetches the whole block from the next
 * level, and evicted dirty blocks are written back to the next level.
 */
void hierarchy_access(
    CacheHierarchy* hierarchy,
    uint64_t address,
    uint64_t size,
    CacheOperation operation
);

#endif
