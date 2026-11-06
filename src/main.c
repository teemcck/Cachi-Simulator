#include <stdio.h>

#include "hierarchy.h"
#include "trace.h"

// Cachi: a Multilevel Processor Cache Simulator.
// Author: Tristan McCaskill
// Date (M/D/Y): 10/18/26
// References: https://www.cs.cornell.edu/courses/cs3410/2026sp/notes/caches

static int simulate_access(uint64_t address, uint64_t size,
                           CacheOperation operation, void* context) {
    hierarchy_access(context, address, size, operation);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <config> <trace>\n", argv[0]);
        return 1;
    }

    CacheHierarchy* hierarchy = create_cache_hierarchy(argv[1]);
    if (hierarchy == NULL) {
        fprintf(stderr, "Could not load cache configuration\n");
        return 1;
    }

    const int result = parse_trace_file(argv[2], simulate_access, hierarchy);
    for (size_t i = 0; i < hierarchy->num_levels; ++i) {
        const CacheStats* stats = &hierarchy->levels[i]->stats;
        const uint64_t accesses = stats->hits + stats->misses;
        const double hit_rate = accesses == 0
            ? 0.0 : 100.0 * (double)stats->hits / (double)accesses;
        const double miss_rate = accesses == 0
            ? 0.0 : 100.0 * (double)stats->misses / (double)accesses;

        printf("L%zu: accesses=%llu hits=%llu misses=%llu "
               "hit_rate=%.2f%% miss_rate=%.2f%% "
               "evictions=%llu writebacks=%llu\n",
               i + 1,
               (unsigned long long)accesses,
               (unsigned long long)stats->hits,
               (unsigned long long)stats->misses,
               hit_rate,
               miss_rate,
               (unsigned long long)stats->evictions,
               (unsigned long long)stats->writebacks);
    }

    free_cache_hierarchy(hierarchy);
    if (result != 0) {
        fprintf(stderr, "Could not parse memory trace\n");
        return 1;
    }
    return 0;
}
