#include "trace.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>

int parse_trace_file(const char* path, TraceCallback callback, void* context) {
    if (path == NULL || callback == NULL) return -1;

    FILE* file = fopen(path, "r");
    if (file == NULL) return -1;

    char line[256];
    int result = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        char operation;
        uint64_t address;
        unsigned long long size;
        int consumed = 0;

        const char* cursor = line;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor == '\0' || *cursor == '#') continue;

        if (sscanf(cursor, " %c %" SCNx64 ",%llu%n",
                   &operation, &address, &size, &consumed) != 3) {
            result = -1;
            break;
        }
        cursor += consumed;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor != '\0' && *cursor != '#') {
            result = -1;
            break;
        }

        if (operation == 'I') continue;
        if (operation == 'L' || operation == 'S') {
            CacheOperation cache_operation = operation == 'L'
                ? CACHE_READ : CACHE_WRITE;
            if (callback(address, size, cache_operation, context) != 0) {
                result = -1;
                break;
            }
        } else if (operation == 'M') {
            if (callback(address, size, CACHE_READ, context) != 0 ||
                callback(address, size, CACHE_WRITE, context) != 0) {
                result = -1;
                break;
            }
        } else {
            result = -1;
            break;
        }
    }

    if (ferror(file)) result = -1;
    fclose(file);
    return result;
}
