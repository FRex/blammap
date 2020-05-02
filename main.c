#include <stdio.h>

#define BLAMMAP_IMPLEMENTATION
#include "blammap.h"

int main(int argc, char ** argv)
{
    blammap_t map;
    int i;

    printf("This is a %d-bit exe!\n", (int)(sizeof(void*) * 8));
    for(i = 0; i < argc; ++i)
    {
        const char * fname = argv[i];
        if(blammap_map(&map, argv[i]))
        {
            printf("OK: mapped %20s, addr = %p, len = %lld\n", fname, map.ptr, map.len);
            blammap_free(&map);
        }
        else
        {
            printf("failed to map %20s: step = %d, name = %s, err = %u, errno = %d\n",
                fname, map.errstep, map.errname, map.errcode, map.linuxerrno);
        }
    } /* for */

    return 0;
}
