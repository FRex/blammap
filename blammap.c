#include "blammap.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void zeroout(blammap_t * map)
{
    assert(map);
    memset(map, 0x0, sizeof(blammap_t));
}

void blammap_init(blammap_t * map)
{
    zeroout(map);
}

static int seterr(blammap_t * map, int step)
{
    assert(map);
    map->errstep = step;
    map->errcode = GetLastError();
    return 1;
}

static int getsize(HANDLE file, long long * out)
{
    LARGE_INTEGER li;

    assert(out);
    if(!GetFileSizeEx(file, &li))
        return 0;

    *out = li.QuadPart;
    return 1;
}

/* this also wouldn't compile if HANDLE was not void ptr so double benefit..? */
static void closeandzerohandle(void ** handle)
{
    assert(handle);
    if(*handle != NULL && *handle != INVALID_HANDLE_VALUE)
        CloseHandle(*handle);

    *handle = NULL;
}

int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname)
{
    zeroout(map);
    while(1)
    {
        map->privfile = CreateFileW(utf16fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(map->privfile == INVALID_HANDLE_VALUE && seterr(map, 2))
            break;

        if(!getsize(map->privfile, &map->len) && seterr(map, 3))
            break;

        map->privmapping = CreateFileMappingW(map->privfile, NULL, PAGE_READONLY, 0, 0, NULL);
        if(map->privmapping == NULL && seterr(map, 4))
            break;

        map->ptr = MapViewOfFile(map->privmapping, FILE_MAP_READ, 0, 0, 0);
        if(!map->ptr && seterr(map, 5))
            break;

        return 1;
    }

    closeandzerohandle(&map->privmapping);
    closeandzerohandle(&map->privfile);
    map->len = 0;
    return 0;
}

int blammap_map(blammap_t * map, const char * utf8fname)
{
#define STACKBUFFSIZE 100
    wchar_t buff[STACKBUFFSIZE];
    wchar_t * buffptr;
    int neededw, ret;

    neededw = MultiByteToWideChar(CP_UTF8, 0u, utf8fname, -1, NULL, 0);
    if(neededw > STACKBUFFSIZE)
        buffptr = calloc(neededw, sizeof(wchar_t));
    else
        buffptr = buff;

    if(!buffptr)
    {
        zeroout(map);
        map->errstep = 1;
        return 0;
    }

    MultiByteToWideChar(CP_UTF8, 0u, utf8fname, -1, buffptr, neededw);
    ret = blammap_map_wide(map, buffptr);
    if(neededw > STACKBUFFSIZE)
        free(buffptr);

#undef STACKBUFFSIZE
    return ret;
}

void blammap_free(blammap_t * map)
{
    assert(map);
    if(map->ptr)
        UnmapViewOfFile(map->ptr);

    closeandzerohandle(&map->privmapping);
    closeandzerohandle(&map->privfile);
    zeroout(map);
}
