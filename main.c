#include <stdio.h>

#define BLAMMAP_IMPLEMENTATION
#include "blammap.h"

int main(int argc, char ** argv)
{
    blammap_t map;
    int i;

    printf("This is a %d-bit exe! sizeof(blammap_t) = %d\n",
        (int)(sizeof(void*) * 8), (int)sizeof(blammap_t));

    for(i = 0; i < argc; ++i)
    {
        const char * fname = argv[i];
        if(blammap_map(&map, argv[i], 0))
        {
            printf("OK: mapped %20s, addr = %p, len = %lld\n", fname, map.ptr, map.len);
            blammap_free(&map);
        }
        else
        {
            printf("failed to map %20s: step = %d, name = %s, errcode = %lld\n",
                fname, map.errstep, map.errname, map.errcode);
        }
    } /* for */

    return 0;
}
