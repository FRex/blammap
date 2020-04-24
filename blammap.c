#include "blammap.h"
#include <assert.h>
#include <string.h>

static void zeroout(blammap_t * map)
{
    assert(map);
    memset(map, 0x0, sizeof(blammap_t));
}

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

static void seterr(blammap_t * map, int step)
{
    assert(map);
    map->errstep = step;
    map->errcode = GetLastError();
}

static int getsize(HANDLE file, long long * out)
{
    LARGE_INTEGER li;
    if(!GetFileSizeEx(file, &li))
        return 0;

    assert(out);
    *out = li.QuadPart;
    return 1;
}

int blammap_map(blammap_t * map, const char * utf8fname)
{
    zeroout(map);
    while(1)
    {
        /* todo: proper utf8->utf16 before this (own func) */
        map->file = CreateFileA(utf8fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(map->file == INVALID_HANDLE_VALUE)
        {
            seterr(map, 1);
            break;
        }

        if(!getsize(map->file, &map->len))
        {
            seterr(map, 2);
            break;
        }

        map->mapping = CreateFileMappingW(map->file, NULL, PAGE_READONLY, 0, 0, NULL);
        if(map->mapping == NULL)
        {
            seterr(map, 3);
            break;
        }

        map->ptr = MapViewOfFile(map->mapping, FILE_MAP_READ, 0, 0, 0);
        if(!map->ptr)
        {
            seterr(map, 4);
            break;
        }

        return 1;
    }

    if(map->mapping)
        CloseHandle(map->mapping);

    if(map->file)
        CloseHandle(map->file);

    map->mapping = NULL;
    map->file = NULL;
    map->len = 0;

    return 0;
}

void blammap_free(blammap_t * map)
{
    /* todo */
}
