
#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"

// Caches are 64 bit addressed by default.
#define ADDRESS_BITS 64

/*
 * Stores individual cache statistics.
 */
typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t writebacks;
} CacheStats;

/*
 * Represents a single block in a cache.
 * 
 * Data is not stored here since a trace analyzer is data-agnostic.
 * Last access, insertion time are remembered for replacement policies.
 */
typedef struct {
    uint64_t tag;
    uint64_t last_access;
    uint64_t insertion_time;
    bool valid;
    bool dirty;
} CacheLine;

/*
 * Stores cache configuration, runtime statistics, and cache metadata.
 */
typedef struct {
    Config config;
    CacheStats stats;
    CacheLine* cache_lines;
    uint64_t access_time;
    // Useful cache properties.
    size_t num_blocks;
    size_t num_sets;
    uint8_t tag_bits;
    uint8_t set_index_bits;
    uint8_t offset_bits;
} Cache;

/*
 * Object used to specify type (r/w) of a cache access.
 */
typedef enum {
    CACHE_READ,
    CACHE_WRITE
} CacheOperation;

/* Result of looking up an address in one cache level. */
typedef enum {
    CACHE_MISS,
    CACHE_HIT
} CacheAccessResult;

/*
 * Describes the line an access evicted, if any.
 * When dirty is true, address is the first byte of the evicted block and
 * the block must be written back to the next level of the hierarchy.
 */
typedef struct {
    bool dirty;
    uint64_t address;
} CacheEviction;

/*
 * Dynamically allocates new cache using provided configuration.
 */
Cache* create_cache(const Config* config);

/*
 * Creates an access request to specified cache.
 * Operation defines read/write/etc.
 *
 * On a miss the block is allocated in this cache. If that evicts a dirty
 * line and eviction is not NULL, eviction describes the block to write back.
 */
CacheAccessResult cache_access(
    Cache *cache,
    uint64_t address,
    CacheOperation operation,
    CacheEviction *eviction
);

/*
 * Returns the address of the first byte in the block containing address.
 */
uint64_t cache_block_address(const Cache *cache, uint64_t address);

/*
 * Destructs a deallocates a cache and its components.
 */
void cache_destroy(Cache *cache);

#endif
