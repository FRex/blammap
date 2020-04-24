typedef struct blammap
{
    void * ptr;
    long long len;

    void * file;
    void * mapping;

    int errstep;
    unsigned errcode;

} blammap_t;

int blammap_map(blammap_t * map, const char * utf8fname);
void blammap_free(blammap_t * map);
