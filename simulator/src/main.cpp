#include "../include/decoder.h"
#include "../include/DemoController.h"
#include <fstream>
#include <array>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Not enough filenames given\n");
        return 1;
    }

    ifstream initial_accesses {argv[1]};
    ifstream trace {argv[2]};

    if (!initial_accesses) {
        printf("No file found by the name %s\n", argv[1]);
        return 2;
    }

    if (!trace) {
        printf("No file found by the name %s\n", argv[2]);
        return 3;
    }

    Decoder decoder = Decoder(initial_accesses, trace);

    constexpr size_t n = 50000;
    array<access, n> accesses_buffer{};
    cout << "let's try read" << endl;
    decoder.read_llc_misses<n>(accesses_buffer, n);
    cout << "decoder read" << endl;
    // TODO: below line should fail...? needs revision
    trace.close();

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