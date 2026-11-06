#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include "cache.h"

/* Called once per cache access parsed from a trace record. */
typedef int (*TraceCallback)(uint64_t address, uint64_t size,
                             CacheOperation operation, void* context);

/*
 * Parses a Valgrind-style trace (I/L/S/M address,size). Instruction fetches
 * are ignored; M records produce a read followed by a write. Returns 0 on
 * success and -1 if the file, a record, or the callback fails.
 */
int parse_trace_file(const char* path, TraceCallback callback, void* context);

#endif
