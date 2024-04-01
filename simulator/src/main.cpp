#include "../include/decoder.h"
#include "../include/simulator.h"
#include "../include/BaselineController.h"
#include "../include/ETMController.h"
#include "../include/MorelloController.h"
#include <fstream>
#include <vector>
#include <unordered_set>

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

    size_t n;
    if (argc >= 5) {
        stringstream str(argv[4]);
        str >> n;
    } else {
        n = 5000000;
    }

    unordered_set<size_t> log_points{};
    ofstream output_log{};
    if (argc >= 6) {
        output_log.open(argv[5]);

        int i = 6;
        while (i < argc) {
            log_points.insert(stoi(argv[i]));
            i++;
        }
    }

    auto accesses_buffer = new vector<access>(n);
    decoder.read_llc_misses(*accesses_buffer, n);
    // TODO: below line should fail...? needs revision (this comment might be stale)
    trace.close();

    ofstream output_trace {argv[3]};
    if (!output_trace) {
        cout << "Could not open " << argv[3] << " for writing" << endl;
        return 4;
    }

    MorelloController controller(decoder, output_trace, output_log, false);
    //ETMController controller(decoder, output_trace);
    //BaselineController controller(output_trace);
    Simulator simulator{};

    cout << "Beginning simulation" << endl;
    size_t count = simulator.processTrace(decoder, controller, *accesses_buffer, n, log_points);
    cout << "Processed " << count << " entries" << endl;

    delete accesses_buffer;

    return 0;
}