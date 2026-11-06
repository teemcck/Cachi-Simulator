#include <stdlib.h>
#include <hierarchy.h>
#include "parse_config.h"

static inline void append_cache(CacheHierarchy* hierarchy, Cache* cache) {
    // Add a cache the the end of the hierarchy and update the number of levels.
    hierarchy->levels[(hierarchy->num_levels)++] = cache;
}

CacheHierarchy* create_cache_hierarchy(const char* input) {
    size_t num_entries;
    Config* configs = parse_config_file(input, &num_entries);
    if (configs == NULL) {
        return NULL;
    }

    // Allocate cache hierarchy.
    CacheHierarchy* hierarchy = (CacheHierarchy*)malloc(sizeof(CacheHierarchy));
    hierarchy->levels = (Cache**)malloc(sizeof(Cache*) * num_entries);
    hierarchy->num_levels = 0;

    // Build caches for each level from config.
    for (size_t i = 0; i < num_entries; ++i) {
        Cache* cache = create_cache(&configs[i]);
        if (cache == NULL) {
            hierarchy = NULL;
            continue;
        }
        append_cache(hierarchy, cache);
    }
    free(configs);
    return hierarchy;
}

void free_cache_hierarchy(CacheHierarchy* hierarchy) {
    if (hierarchy == NULL) return;
    for (size_t i = 0; i < hierarchy->num_levels; ++i) {
        // Free individual caches stored in hierarchy.
        cache_destroy(hierarchy->levels[i]);
    }
    // Free hierarchy data structure.
    free(hierarchy->levels);
    free(hierarchy);
}

static void access_level(
    CacheHierarchy* hierarchy,
    size_t level,
    uint64_t address,
    uint64_t size,
    CacheOperation operation
) {
    // Accesses that fall out of the last level go to memory, which is not modeled.
    if (level >= hierarchy->num_levels) return;
    if (size == 0) size = 1;

    Cache* cache = hierarchy->levels[level];
    const uint64_t block_size = (uint64_t)cache->config.block_size;
    const uint64_t last_byte = address + (size - 1);

    // Visit every block this access touches, one request per block.
    uint64_t block = cache_block_address(cache, address);
    const uint64_t last_block = cache_block_address(cache, last_byte);
    while (1) {
        const uint64_t request = block > address ? block : address;

        CacheEviction eviction;
        const CacheAccessResult result =
            cache_access(cache, request, operation, &eviction);

        // A dirty victim is written back to the next level as a full block.
        if (eviction.dirty) {
            access_level(hierarchy, level + 1, eviction.address,
                         block_size, CACHE_WRITE);
        }
        // A miss fetches the whole block from the next level.
        if (result == CACHE_MISS) {
            access_level(hierarchy, level + 1, block, block_size, CACHE_READ);
        }

        if (block == last_block) break;
        block += block_size;
    }
}

void hierarchy_access(
    CacheHierarchy* hierarchy,
    uint64_t address,
    uint64_t size,
    CacheOperation operation
) {
    if (hierarchy == NULL) return;
    access_level(hierarchy, 0, address, size, operation);
}
