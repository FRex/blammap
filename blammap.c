#include "blammap.h"
#include <assert.h>
#include <string.h>

static void zero_out(blammap_t * map)
{
    assert(map);
    memset(map, 0x0, sizeof(blammap_t));
}

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

int blammap_map(blammap_t * map, const char * utf8fname)
{
    LARGE_INTEGER li;
    HANDLE f, m;
    void * ptr;

    zero_out(map);

    /* todo: proper utf8->utf16 before this (own func) */
    f = CreateFileA(utf8fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(f == INVALID_HANDLE_VALUE)
    {
        map->errstep = 1;
        map->errcode = GetLastError();
        return 0;
    }

    if(GetFileSizeEx(f, &li) == 0)
    {
        CloseHandle(f);
        map->errstep = 2;
        map->errcode = GetLastError();
        return 0;
    }

    m = CreateFileMappingW(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if(m == NULL)
    {
        CloseHandle(f);
        map->errstep = 3;
        map->errcode = GetLastError();
        return 0;
    }

    ptr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    if(!ptr)
    {
        CloseHandle(f);
        CloseHandle(m);
        map->errstep = 3;
        map->errcode = GetLastError();
        return 0;
    }

    map->ptr = ptr;
    map->len = li.QuadPart;

    map->file = f;
    map->mapping = m;

    map->errstep = 0;
    map->errcode = 0;

    return 1;
}

void blammap_free(blammap_t * map)
{
    /* todo */
}
