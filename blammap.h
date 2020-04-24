#ifndef BLAMMAP_H
#define BLAMMAP_H

typedef struct blammap
{
    /* ptr and len of the mapping */
    void * ptr;
    long long len;

    /* function number that failed + error code from winapi */
    int errstep;
    unsigned errcode;

    /* private, for unmapping, don't touch */
    void * privfile;
    void * privmapping;

} blammap_t;

/* zeroes out the struct to make it a noop when passed to free, idempotent */
void blammap_init(blammap_t * map);

/* unmaps the mapping and zeroes out the struct, noop if there was no map since last free/init, idempotent */
void blammap_free(blammap_t * map);

/* mmaps a file, returns 1 on success, 0 on error, doesn't free a previous mapping */
int blammap_map(blammap_t * map, const char * utf8fname);

/* TODO: put ifdef around here */
#include <wchar.h>
int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname);

#endif /* BLAMMAP_H */

#ifdef BLAMMAP_IMPLEMENTATION

/* ... */

#endif /* BLAMMAP_IMPLEMENTATION */
