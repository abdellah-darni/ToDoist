#include <stdlib.h>
#include <string.h>

#include "utils.h"

char *trim_fieldbuf(const char *buf){
    if (!buf) return strdup("");

    const char *start = buf;
    while(*start && *start == ' ') start++;

    const char *end = start + strlen(start);
    while(end > start && (*(end-1) == ' ' || *(end-1) == '\r' || *(end-1) == '\n')) end--;

    size_t len = (size_t)(end - start);

    char *out = malloc(len+1);
    if(!out) return NULL;

    memcpy(out, start, len);
    out[len] = '\0';

    return out;
}