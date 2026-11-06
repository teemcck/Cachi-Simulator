#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"
#include "hierarchy.h"
#include "trace.h"

static Config config(size_t capacity, size_t block_size, size_t associativity,
                     ReplacementType replacement) {
    Config result = {
        .capacity = (int)capacity,
        .block_size = (int)block_size,
        .associativity = (int)associativity,
        .replacement = replacement
    };
    return result;
}

static void test_config_validation(void) {
    Config valid = config(128, 64, 2, LRU);
    Config invalid_block_size = config(128, 48, 2, LRU);
    Config invalid_capacity = config(96, 64, 1, LRU);

    Cache* cache = create_cache(&valid);
    assert(cache != NULL);
    assert(cache->num_sets == 1);
    cache_destroy(cache);

    assert(create_cache(&invalid_block_size) == NULL);
    assert(create_cache(&invalid_capacity) == NULL);
    assert(create_cache(NULL) == NULL);
}

static void test_hit_miss_and_dirty_eviction(void) {
    Config cfg = config(128, 64, 2, LRU);
    Cache* cache = create_cache(&cfg);
    assert(cache != NULL);

    assert(cache_access(cache, 0, CACHE_WRITE, NULL) == CACHE_MISS);
    assert(cache_access(cache, 64, CACHE_WRITE, NULL) == CACHE_MISS);
    assert(cache_access(cache, 0, CACHE_READ, NULL) == CACHE_HIT);
    assert(cache_access(cache, 128, CACHE_READ, NULL) == CACHE_MISS);
    assert(cache->stats.hits == 1);
    assert(cache->stats.misses == 3);
    assert(cache->stats.evictions == 1);
    assert(cache->stats.writebacks == 1);

    cache_destroy(cache);
}

static void test_lru_replacement(void) {
    Config cfg = config(128, 64, 2, LRU);
    Cache* cache = create_cache(&cfg);
    assert(cache != NULL);

    cache_access(cache, 0, CACHE_READ, NULL);
    cache_access(cache, 64, CACHE_READ, NULL);
    cache_access(cache, 0, CACHE_READ, NULL);
    cache_access(cache, 128, CACHE_READ, NULL);
    assert(cache->cache_lines[0].tag == 0);
    assert(cache->cache_lines[1].tag == 2);

    cache_destroy(cache);
}

static void test_fifo_replacement(void) {
    Config cfg = config(128, 64, 2, FIFO);
    Cache* cache = create_cache(&cfg);
    assert(cache != NULL);

    cache_access(cache, 0, CACHE_READ, NULL);
    cache_access(cache, 64, CACHE_READ, NULL);
    cache_access(cache, 64, CACHE_READ, NULL);
    cache_access(cache, 128, CACHE_READ, NULL);
    assert(cache->cache_lines[0].tag == 2);
    assert(cache->cache_lines[1].tag == 1);

    cache_destroy(cache);
}

static void test_mru_replacement(void) {
    Config cfg = config(128, 64, 2, MRU);
    Cache* cache = create_cache(&cfg);
    assert(cache != NULL);

    cache_access(cache, 0, CACHE_READ, NULL);
    cache_access(cache, 64, CACHE_READ, NULL);
    cache_access(cache, 64, CACHE_READ, NULL);
    cache_access(cache, 128, CACHE_READ, NULL);
    assert(cache->cache_lines[0].tag == 0);
    assert(cache->cache_lines[1].tag == 2);

    cache_destroy(cache);
}

static void test_hierarchy_stops_after_hit(void) {
    Config l1_config = config(64, 64, 1, LRU);
    Config l2_config = config(128, 64, 2, LRU);
    Cache* levels[] = { create_cache(&l1_config), create_cache(&l2_config) };
    assert(levels[0] != NULL && levels[1] != NULL);
    CacheHierarchy hierarchy = { .levels = levels, .num_levels = 2 };

    hierarchy_access(&hierarchy, 0, 8, CACHE_READ);
    assert(levels[0]->stats.misses == 1);
    assert(levels[1]->stats.misses == 1);

    hierarchy_access(&hierarchy, 0, 8, CACHE_READ);
    assert(levels[0]->stats.hits == 1);
    assert(levels[1]->stats.hits == 0);

    cache_destroy(levels[0]);
    cache_destroy(levels[1]);
}

static void test_eviction_reports_dirty_block(void) {
    // Direct-mapped, two sets of 64-byte blocks: 0 and 128 share set 0.
    Config cfg = config(128, 64, 1, LRU);
    Cache* cache = create_cache(&cfg);
    assert(cache != NULL);

    CacheEviction eviction;
    assert(cache_access(cache, 0x10, CACHE_WRITE, &eviction) == CACHE_MISS);
    assert(!eviction.dirty);
    assert(cache_access(cache, 0x90, CACHE_READ, &eviction) == CACHE_MISS);
    assert(eviction.dirty);
    assert(eviction.address == 0);
    assert(cache_access(cache, 0x10, CACHE_READ, &eviction) == CACHE_MISS);
    assert(!eviction.dirty);

    cache_destroy(cache);
}

static void test_straddling_access_is_split(void) {
    Config l1_config = config(128, 64, 2, LRU);
    Config l2_config = config(256, 128, 2, LRU);
    Cache* levels[] = { create_cache(&l1_config), create_cache(&l2_config) };
    assert(levels[0] != NULL && levels[1] != NULL);
    CacheHierarchy hierarchy = { .levels = levels, .num_levels = 2 };

    // Bytes 60..67 cross the 64-byte boundary: two L1 requests.
    hierarchy_access(&hierarchy, 60, 8, CACHE_READ);
    assert(levels[0]->stats.misses == 2);
    // Both 64-byte fills land in the same 128-byte L2 block.
    assert(levels[1]->stats.misses == 1);
    assert(levels[1]->stats.hits == 1);

    // An access inside one block stays a single request.
    hierarchy_access(&hierarchy, 0, 8, CACHE_READ);
    assert(levels[0]->stats.hits == 1);
    assert(levels[0]->stats.misses == 2);

    cache_destroy(levels[0]);
    cache_destroy(levels[1]);
}

static void test_dirty_writeback_reaches_next_level(void) {
    Config l1_config = config(64, 64, 1, LRU);
    Config l2_config = config(256, 64, 4, LRU);
    Cache* levels[] = { create_cache(&l1_config), create_cache(&l2_config) };
    assert(levels[0] != NULL && levels[1] != NULL);
    CacheHierarchy hierarchy = { .levels = levels, .num_levels = 2 };

    // Write miss: L1 allocates a dirty line, L2 only sees a clean fetch.
    hierarchy_access(&hierarchy, 0, 8, CACHE_WRITE);
    assert(levels[1]->stats.misses == 1);
    assert(!levels[1]->cache_lines[0].dirty);

    // Reading block 64 evicts dirty block 0 from L1 and writes it to L2.
    hierarchy_access(&hierarchy, 64, 8, CACHE_READ);
    assert(levels[0]->stats.writebacks == 1);
    assert(levels[1]->stats.hits == 1);
    assert(levels[1]->cache_lines[0].tag == 0);
    assert(levels[1]->cache_lines[0].dirty);
    assert(levels[1]->stats.misses == 2);

    cache_destroy(levels[0]);
    cache_destroy(levels[1]);
}

typedef struct {
    uint64_t addresses[8];
    uint64_t sizes[8];
    CacheOperation operations[8];
    size_t count;
} CapturedTrace;

static int capture_access(uint64_t address, uint64_t size,
                          CacheOperation operation, void* context) {
    CapturedTrace* captured = context;
    assert(captured->count < 8);
    captured->addresses[captured->count] = address;
    captured->sizes[captured->count] = size;
    captured->operations[captured->count] = operation;
    ++captured->count;
    return 0;
}

static void test_trace_parser(void) {
    const char* path = "tests/fixtures/basic.trace";
    CapturedTrace captured = {0};

    assert(parse_trace_file(path, capture_access, &captured) == 0);
    assert(captured.count == 4);
    assert(captured.addresses[0] == 0x10 &&
           captured.operations[0] == CACHE_READ);
    assert(captured.sizes[0] > 0);
    assert(captured.addresses[1] == 0x20 &&
           captured.operations[1] == CACHE_WRITE);
    assert(captured.addresses[2] == 0x30 &&
           captured.operations[2] == CACHE_READ);
    assert(captured.addresses[3] == 0x30 &&
           captured.operations[3] == CACHE_WRITE);

    assert(parse_trace_file("tests/fixtures/invalid.trace",
                            capture_access, &captured) == -1);
}

int main(void) {
    test_config_validation();
    test_hit_miss_and_dirty_eviction();
    test_lru_replacement();
    test_fifo_replacement();
    test_mru_replacement();
    test_hierarchy_stops_after_hit();
    test_eviction_reports_dirty_block();
    test_straddling_access_is_split();
    test_dirty_writeback_reaches_next_level();
    test_trace_parser();
    puts("All tests passed.");
    return 0;
}
