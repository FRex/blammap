#include <stdio.h>
#include "blammap.h"

int main(int argc, char ** argv)
{
    blammap_t map;
    const char * fname;
    int i;

    printf("This is a %d-bit exe!\n", (int)(sizeof(void*) * 8));
    for(i = 0; i < argc; ++i)
    {
        fname = argv[i];
        if(blammap_map(&map, argv[i]))
        {
            printf("OK: mapped %20s, addr = %p, len = %lld\n", fname, map.ptr, map.len);
            blammap_free(&map);
        }
        else
        {
            printf("failed to map %20s: step = %d, err = %u\n", fname, map.errstep, map.errcode);
        }
    } /* for */

    return 0;
}
