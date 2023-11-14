#include "decoder.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Not enough filenames given\n");
        return 1;
    }

    FILE *initial_accesses = fopen(argv[1], "rb");
    FILE *trace = fopen(argv[2], "rb");

    if (!initial_accesses) {
        printf("No file found by the name %s\n", argv[1]);
        return 2;
    }

    if (!trace) {
        printf("No file found by the name %s\n", argv[2]);
        return 3;
    }

    int n = 5000;
    initial_access buf[n];
    read_init_accesses(initial_accesses, buf, n);

    printf("Entries have size %lu\n", sizeof(initial_access));
    printf("Initial accesses are as follows\n");
    for (int i = 0; i < n; i++) {
        initial_access r = buf[i];
        if (1) {
            printf("%d: %d, %d, \n",
                   i,
                   r.type,
                   r.tag
            );
        }
    }

    printf("SWITCH\n");

    n = 5000;
    llc_miss buf3[n];
    read_llc_misses(trace, buf3, n);

    printf("Entries have size %lu\n", sizeof(llc_miss));
    printf("LLC misses are as follows\n");
    for (int i = 0; i < n; i++) {
        llc_miss r = buf3[i];
        if (1) {
            printf("%d: %s, %d, %d, %d, %lu\n",
                   i,
                   (r.type == LLC_MISS_TYPE_READ) ? "R" : "W",
                   r.size,
                   r.tags,
                   r.tags_known,
                   r.addr
            );
        }
    }


    fclose(trace);

    return 0;
}
