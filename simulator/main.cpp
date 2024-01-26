//
// Created by kofi on 29/12/23.
//

#include "decoder.h"
#include "DemoController.h"
#include <fstream>

int main(int argc, char *argv[]) {
    if (argc < 4) {
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

    Decoder decoder = Decoder(initial_accesses, trace);

    int n = 50000;
    /*
    initialAccess initial_accesses_buffer[n];
    decoder.read_init_accesses(initial_accesses_buffer, n);
    fclose(initial_accesses);

    printf("Entries have size %lu\n", sizeof(initialAccess));
    printf("Initial accesses are as follows\n");
    for (int i = 0; i < n; i++) {
        initialAccess r = initial_accesses_buffer[i];
        if (0) {
            printf("%d: %d, %d, \n",
                   i,
                   r.type,
                   r.tag
            );
        }
    }

    printf("SWITCH\n");
     */

    access accesses_buffer[n];
    cout << "let's try read" << endl;
    decoder.read_llc_misses(accesses_buffer, n);
    cout << "decoder read" << endl;
    fclose(trace);

    /*
    printf("Entries have size %lu\n", sizeof(llcMiss));
    printf("LLC misses are as follows\n");
    for (int i = 0; i < n; i++) {
        access r = accesses_buffer[i];
        if (1) {
            printf("%d: %s, %d, %d, %lu\n",
                   i,
                   (r.type == ACCESS_TYPE_READ) ? "R" : "W",
                   r.size,
                   r.tags,
                   r.addr
            );
        }
    }
    */

    ofstream output_trace {argv[3]};
    if (!output_trace) {
        printf("Could not open %s for writing\n", argv[3]);
        return 4;
    }

    DemoController controller(output_trace);
    Simulator simulator{};

    printf("Beginning simulation\n");
    size_t count = simulator.processTrace(decoder, controller, accesses_buffer, n);

    cout << "Processed " << count << " entries" << endl;

    return 0;
}