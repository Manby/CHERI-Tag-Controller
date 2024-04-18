#include "../include/decoder.h"
#include "../include/simulator.h"
#include "../include/BaselineController.h"
#include "../include/ETMController.h"
#include "../include/MorelloController.h"
#include <fstream>
#include <vector>
#include <unordered_set>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 5) {
        cout << "Not enough arguments given" << endl;
        return 1;
    }


    ifstream initial_accesses {argv[2]};
    ifstream trace {argv[3]};

    if (!initial_accesses) {
        cout << "No file found by the name " << argv[2] << endl;
        return 2;
    }

    if (!trace) {
        cout << "No file found by the name " << argv[3] << endl;
        return 3;
    }

    cout << "Decoding trace file" << endl;
    Decoder decoder = Decoder(initial_accesses, trace);

    size_t n;
    if (argc >= 6) {
        stringstream str(argv[5]);
        str >> n;
    } else {
        n = 5000000;
    }

    unordered_set<size_t> log_points{};
    ofstream output_log{};
    if (argc >= 7) {
        output_log.open(argv[6]);

        int i = 7;
        while (i < argc) {
            log_points.insert(stoi(argv[i]));
            i++;
        }
    }

    auto accesses_buffer = new vector<memAccess>(n);
    decoder.read_llc_misses(*accesses_buffer, n);
    // TODO: below line should fail...? needs revision (this comment might be stale)
    trace.close();

    ofstream output_trace {argv[4]};
    if (!output_trace) {
        cout << "Could not open " << argv[4] << " for writing" << endl;
        return 4;
    }

    Simulator simulator{};
    size_t count;

    cout << "Beginning simulation using ";

    if (!strcmp(argv[1], "morello-t")) {
        cout << "Morello tag controller implementation, Tag Cache" << endl;
        MorelloController controller(decoder, output_trace, output_log, true);
        count = simulator.processTrace(decoder, controller, *accesses_buffer, n, log_points);
        cout << "Performed [" << controller.get_num_accesses() << "] accesses" << endl;
    } else if (!strcmp(argv[1], "morello-z")) {
        cout << "Morello tag controller implementation, Zero Cache" << endl;
        MorelloController controller(decoder, output_trace, output_log, false);
        count = simulator.processTrace(decoder, controller, *accesses_buffer, n, log_points);
        cout << "Performed [" << controller.get_num_accesses() << "] accesses" << endl;
    } else if (!strcmp(argv[1], "etm")) {
        cout << "ETM tag controller implementation" << endl;
        ETMController controller(decoder, output_trace, output_log);
        count = simulator.processTrace(decoder, controller, *accesses_buffer, n, log_points);
        cout << "Performed [" << controller.get_num_accesses() << "] accesses" << endl;
    } else {
        cout << "Baseline tag controller implementation" << endl;
        BaselineController controller(decoder, output_trace, output_log);
        count = simulator.processTrace(decoder, controller, *accesses_buffer, n, log_points);
        cout << "Performed [" << controller.get_num_accesses() << "] accesses" << endl;
    }

    cout << "Processed " << count << " entries" << endl;

    delete accesses_buffer;

    return 0;
}