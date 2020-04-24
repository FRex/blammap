typedef struct blammap
{
    void * ptr;
    long long len;

    void * file;
    void * mapping;

    int errstep;
    unsigned errcode;

} blammap_t;

void blammap_init(blammap_t * map);
void blammap_free(blammap_t * map);
int blammap_map(blammap_t * map, const char * utf8fname);
