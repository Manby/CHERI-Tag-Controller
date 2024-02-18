#include "../include/decoder.h"
#include "../include/simulator.h"
#include "../include/BaselineController.h"
//#include "../include/ETMController.h"
#include <fstream>
#include <array>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Not enough filenames given" << endl;
        return 1;
    }

    ifstream initial_accesses {argv[1]};
    ifstream trace {argv[2]};

    if (!initial_accesses) {
        cout << "No file found by the name " << argv[1] << endl;
        return 2;
    }

    if (!trace) {
        cout << "No file found by the name " << argv[2] << endl;
        return 3;
    }

    cout << "Decoding trace file" << endl;
    Decoder decoder = Decoder(initial_accesses, trace);

    constexpr size_t n = 5000000;
    auto accesses_buffer = new array<access, n>{};
    decoder.read_llc_misses(*accesses_buffer, n);
    // TODO: below line should fail...? needs revision (this comment might be stale)
    trace.close();

    ofstream output_trace {argv[3]};
    if (!output_trace) {
        cout << "Could not open " << argv[3] << " for writing" << endl;
        return 4;
    }

    //ETMController controller(decoder, output_trace);
    BaselineController controller(output_trace);
    Simulator simulator{};

    cout << "Beginning simulation" << endl;
    size_t count = simulator.processTrace(decoder, controller, *accesses_buffer, n);
    cout << "Processed " << count << " entries" << endl;

    delete accesses_buffer;

    return 0;
}