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
    /* is this a succesful mapping, 0 = not so zeroed struct is noop to free */
    int ok;

    /* ptr and len of the mapping */
    void * ptr;
    long long len;

    /* function number that failed + name of function that failed + error code from winapi/errno */
    int errstep;
    const char * errname;
    long long errcode;

    /* private, for open/close/mapping/unmapping, don't touch */
    void * privfile;
    void * privmapping;

} blammap_t;

/* zeroes out the struct to make it a noop when passed to free, idempotent */
void blammap_init(blammap_t * map);

/* unmaps the mapping and zeroes out the struct, noop if there was no map since last free/init, idempotent */
void blammap_free(blammap_t * map);

/* mmaps a file, returns 1 on success, 0 on error, doesn't free a previous mapping */
int blammap_map(blammap_t * map, const char * utf8fname, long long maxfsize);

#ifdef BLAMMAP_WINDOWS
#include <wchar.h>
int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname, long long maxfsize);
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

/* return 0 if the fsize is too big to map compared to allowed */
static int blammap_priv_checksize(long long maxfsize, long long fsize)
{
    if(maxfsize <= 0)
        return 1;

    return fsize <= maxfsize;
}

#define BLAMMAP_PRIV_STATIC_ASSERT(msg, expr) typedef int BLAMMAP_PRIV_STATIC_ASSERT_##msg[(expr) * 2 - 1];
BLAMMAP_PRIV_STATIC_ASSERT(long_long_is_64_bit, sizeof(long long) == 8);
#undef BLAMMAP_PRIV_STATIC_ASSERT

/* windows specific impl: */
#ifdef BLAMMAP_WINDOWS
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static int blammap_priv_seterr_code(blammap_t * map, int step, const char * name, DWORD err)
{
    assert(map);
    map->errstep = step;
    map->errname = name;
    map->errcode = err;
    return 1;
}

static int blammap_priv_seterr(blammap_t * map, int step, const char * name)
{
    return blammap_priv_seterr_code(map, step, name, GetLastError());
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

int blammap_map_wide(blammap_t * map, const wchar_t * utf16fname, long long maxfsize)
{
    blammap_priv_zeroout(map);
    while(1)
    {
        map->privfile = CreateFileW(utf16fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(map->privfile == INVALID_HANDLE_VALUE && blammap_priv_seterr(map, 1, "CreateFileW"))
            break;

        if(!blammap_priv_getsize(map->privfile, &map->len) && blammap_priv_seterr(map, 2, "GetFileSizeEx"))
            break;

        if(!blammap_priv_checksize(maxfsize, map->len) && blammap_priv_seterr_code(map, 3, "checksize", 0))
            break;

        /* todo: fix the confusing error this gives for empty files, special case empty file maybe? */
        map->privmapping = CreateFileMappingW(map->privfile, NULL, PAGE_READONLY, 0, 0, NULL);
        if(map->privmapping == NULL && blammap_priv_seterr(map, 4, "CreateFileMappingW"))
            break;

        map->ptr = MapViewOfFile(map->privmapping, FILE_MAP_READ, 0, 0, 0);
        if(!map->ptr && blammap_priv_seterr(map, 5, "MapViewOfFile"))
            break;

        map->ok = 1;
        return 1;
    }

    blammap_priv_closeandzerohandle(&map->privmapping);
    blammap_priv_closeandzerohandle(&map->privfile);
    map->ptr = NULL;
    map->len = 0;
    map->ok = 0;
    return 0;
}

int blammap_map(blammap_t * map, const char * utf8fname, long long maxfsize)
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
        map->errname = "calloc";
        return 0;
    }

    MultiByteToWideChar(CP_UTF8, 0u, utf8fname, -1, buffptr, neededw);
    ret = blammap_map_wide(map, buffptr, maxfsize);
    if(neededw > BLAMMAP_PRIV_STACKBUFFSIZE)
        free(buffptr);

#undef BLAMMAP_PRIV_STACKBUFFSIZE
    return ret;
}

void blammap_free(blammap_t * map)
{
    if(!map || !map->ok)
        return;

    if(map->ptr)
        UnmapViewOfFile(map->ptr);

    blammap_priv_closeandzerohandle(&map->privmapping);
    blammap_priv_closeandzerohandle(&map->privfile);
    blammap_priv_zeroout(map);
}

#endif /* BLAMMAP_WINDOWS */

/* posix specific impl: */
#ifdef BLAMMAP_POSIX

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int blammap_priv_seterr_code(blammap_t * map, int step, const char * name, int err)
{
    assert(map);
    map->errstep = step;
    map->errname = name;
    map->errcode = err;
    return 1;
}

static int blammap_priv_seterr(blammap_t * map, int step, const char * name)
{
    return blammap_priv_seterr_code(map, step, name, errno);
}

#define BLAMMAP_PRIV_STATIC_ASSERT(msg, expr) typedef int BLAMMAP_PRIV_STATIC_ASSERT_##msg[(expr) * 2 - 1];
BLAMMAP_PRIV_STATIC_ASSERT(off_t_is_64_bit, sizeof(off_t) == 8);
#undef BLAMMAP_PRIV_STATIC_ASSERT

/* only compiles if st_size in struct stat is off_t so with above static assert it ensures 64-bit fsize */
static long long blammap_priv_cast(off_t * size)
{
    return (long long)(*size);
}

static int blammap_priv_getsize(int fd, long long * out)
{
    struct stat mybuf;
    assert(out);
    if(fstat(fd, &mybuf))
        return 0;

    *out = blammap_priv_cast(&mybuf.st_size);
    return 1;
}

int blammap_map(blammap_t * map, const char * utf8fname, long long maxfsize)
{
    int fd;

    blammap_priv_zeroout(map);

    fd = -1;
    while(1)
    {
        fd = open(utf8fname, O_RDONLY);
        if(fd == -1 && blammap_priv_seterr(map, 1, "open"))
            break;

        if(!blammap_priv_getsize(fd, &map->len) && blammap_priv_seterr(map, 2, "fstat"))
            break;

        if(!blammap_priv_checksize(maxfsize, map->len) && blammap_priv_seterr_code(map, 3, "checksize", 0))
            break;

        /* todo: this fails also with size 0 */
        map->ptr = mmap(NULL, (size_t)map->len, PROT_READ, MAP_PRIVATE, fd, 0);
        if(map->ptr == MAP_FAILED && blammap_priv_seterr(map, 4, "mmap"))
            break;

        /* after mapping is done we can close the fd */
        close(fd);

        map->ok = 1;
        return 1;
    }

    if(fd != -1)
        close(fd);

    map->ptr = NULL;
    map->len = 0;
    map->ok = 0;
    return 0;
}

void blammap_free(blammap_t * map)
{
    if(!map || !map->ok)
        return;

    if(map->ptr)
        munmap(map->ptr, map->len);

    blammap_priv_zeroout(map);
}

#endif /* BLAMMAP_POSIX */

#endif /* BLAMMAP_IMPLEMENTATION */
