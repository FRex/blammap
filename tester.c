#include <stdio.h>

#define BLAMMAP_IMPLEMENTATION
#include "blammap.h"

int main(int argc, char ** argv)
{
    blammap_t map;
    int i, count, fails;

    printf("This is a %d-bit exe! sizeof(blammap_t) = %d\n",
        (int)(sizeof(void*) * 8), (int)sizeof(blammap_t));

    count = 12345;
    printf("mapping argv[0] %d times\n", count);

    fails = 0;

    for(i = 0; i < count; ++i)
    {
        if(!blammap_map(&map, argv[0]))
        {
            printf("FAILED: i = %d, step = %d, name = %s, errcode = %lld\n",
                i, map.errstep, map.errname, map.errcode);

            ++fails;
            continue;
        }

        blammap_free(&map);
    }

    printf("Done, %d fails\n", fails);
    return 0;
}
