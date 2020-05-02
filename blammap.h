#ifndef BLAMMAP_H
#define BLAMMAP_H

#if !defined(BLAMMAP_WINDOWS) && !defined(BLAMMAP_POSIX)
#ifdef _MSC_VER
#define BLAMMAP_WINDOWS
#else
#define BLAMMAP_POSIX
#endif
#endif /* blamap windows and posix both not defined */

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

#ifdef BLAMMAP_WINDOWS
#include <wchar.h>
int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname);
#endif /* BLAMMAP_WINDOWS */

#endif /* BLAMMAP_H */

#ifdef BLAMMAP_IMPLEMENTATION

/* common impl part: */
#include <string.h>
#include <assert.h>
static void blammap_priv_zeroout(blammap_t * map)
{
    assert(map);
    memset(map, 0x0, sizeof(blammap_t));
}

void blammap_init(blammap_t * map)
{
    blammap_priv_zeroout(map);
}

/* windows specific impl: */
#ifdef BLAMMAP_WINDOWS
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static int blammap_priv_seterr(blammap_t * map, int step)
{
    assert(map);
    map->errstep = step;
    map->errcode = GetLastError();
    return 1;
}

static int blammap_priv_getsize(HANDLE file, long long * out)
{
    LARGE_INTEGER li;

    assert(out);
    if(!GetFileSizeEx(file, &li))
        return 0;

    *out = li.QuadPart;
    return 1;
}

/* this also wouldn't compile if HANDLE was not void ptr so double benefit..? */
static void blammap_priv_closeandzerohandle(void ** handle)
{
    assert(handle);
    if(*handle != NULL && *handle != INVALID_HANDLE_VALUE)
        CloseHandle(*handle);

    *handle = NULL;
}

int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname)
{
    blammap_priv_zeroout(map);
    while(1)
    {
        map->privfile = CreateFileW(utf16fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(map->privfile == INVALID_HANDLE_VALUE && blammap_priv_seterr(map, 2))
            break;

        if(!blammap_priv_getsize(map->privfile, &map->len) && blammap_priv_seterr(map, 3))
            break;

        /* todo: fix the confusing error this gives for empty files, special case empty file maybe? */
        map->privmapping = CreateFileMappingW(map->privfile, NULL, PAGE_READONLY, 0, 0, NULL);
        if(map->privmapping == NULL && blammap_priv_seterr(map, 4))
            break;

        map->ptr = MapViewOfFile(map->privmapping, FILE_MAP_READ, 0, 0, 0);
        if(!map->ptr && blammap_priv_seterr(map, 5))
            break;

        return 1;
    }

    blammap_priv_closeandzerohandle(&map->privmapping);
    blammap_priv_closeandzerohandle(&map->privfile);
    map->len = 0;
    return 0;
}

int blammap_map(blammap_t * map, const char * utf8fname)
{
#define BLAMMAP_PRIV_STACKBUFFSIZE 100
    wchar_t buff[BLAMMAP_PRIV_STACKBUFFSIZE];
    wchar_t * buffptr;
    int neededw, ret;

    neededw = MultiByteToWideChar(CP_UTF8, 0u, utf8fname, -1, NULL, 0);
    if(neededw > BLAMMAP_PRIV_STACKBUFFSIZE)
        buffptr = (wchar_t*)calloc(neededw, sizeof(wchar_t));
    else
        buffptr = buff;

    if(!buffptr)
    {
        blammap_priv_zeroout(map);
        map->errstep = 1;
        return 0;
    }

    MultiByteToWideChar(CP_UTF8, 0u, utf8fname, -1, buffptr, neededw);
    ret = blammap_map_wide(map, buffptr);
    if(neededw > BLAMMAP_PRIV_STACKBUFFSIZE)
        free(buffptr);

#undef BLAMMAP_PRIV_STACKBUFFSIZE
    return ret;
}

void blammap_free(blammap_t * map)
{
    assert(map);
    if(map->ptr)
        UnmapViewOfFile(map->ptr);

    blammap_priv_closeandzerohandle(&map->privmapping);
    blammap_priv_closeandzerohandle(&map->privfile);
    blammap_priv_zeroout(map);
}

#endif /* BLAMMAP_WINDOWS */

/* posix specific impl: */
#ifdef BLAMMAP_POSIX
#error "BLAMMAP_POSIX not implemented!"
#endif /* BLAMMAP_POSIX */

#endif /* BLAMMAP_IMPLEMENTATION */
