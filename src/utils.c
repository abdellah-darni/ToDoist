#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

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


int validate_datetime(const char *s){
    if (!s) return 0;

    int hour, min, day, month, year;

    int got = sscanf(s, "%2d:%2d %2d/%2d/%4d", &hour, &min, &day, &month, &year);

    if (got != 5) return 0;

    // time_t now = time(NULL);  i'm still thinking about it
    // struct tm *t = localtime(&now);
    // int current_year = t->tm_year + 1900;
    // int current_month = t->tm_mon + 1;
    // int current_day = t->tm_mday;
    // int current_hour = t->tm_hour;
    // int current_min = t->tm_min;

    
    if (hour < 0 || hour > 23) return 0;
    if (min < 0 || min > 59) return 0;
    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > 31) return 0;
    if (year < 1900 || year > 9999) return 0;
    return 1;

}
