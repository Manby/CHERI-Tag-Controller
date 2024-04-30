#include "../include/Decoder.h"
#include "../include/Simulator.h"
#include "../include/BaselineController.h"
#include "../include/ETMController.h"
#include "../include/MorelloController.h"
#include "../include/PhoenixController.h"
#include <fstream>
#include <unordered_set>
#include <string.h>

bool endsWithDotGZ(string s) {
    if (s.size() < 3) return false;
    if (s[s.size()-3] != '.') return false;
    if (s[s.size()-2] != 'g') return false;
    if (s[s.size()-1] != 'z') return false;

    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        cout << "Not enough arguments given" << endl;
        return 1;
    }

    ifstream initial_accesses{argv[2]};
    gzFile trace = gzopen(argv[3], "rb");

    if (!initial_accesses) {
        cout << "No file found by the name " << argv[2] << endl;
        return 2;
    }

    if (!trace) {
        cout << "No file found by the name " << argv[3] << endl;
        return 3;
    }

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

    string output_trace_filename = string(argv[4]);
    if (!endsWithDotGZ(output_trace_filename)) output_trace_filename.append(".gz");
    gzFile output_trace = gzopen(output_trace_filename.c_str(), "wb");
    if (!output_trace) {
        cout << "Could not open " << argv[4] << " for writing" << endl;
        return 4;
    }

    size_t count;

    cout << "Beginning simulation using ";

    if (!strcmp(argv[1], "morello-t")) {
        cout << "Morello tag controller implementation, Tag Cache" << endl;
        MorelloController controller(output_trace, output_log, true);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "morello-z")) {
        cout << "Morello tag controller implementation, Zero Cache" << endl;
        MorelloController controller(output_trace, output_log, false);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "etm")) {
        cout << "ETM tag controller implementation" << endl;
        ETMController controller(output_trace, output_log);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-8t")) {
        cout << "Phoenix tag controller implementation; TWS=8, True LRU" << endl;
        PhoenixController controller(output_trace, output_log, 8, true);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-8f")) {
        cout << "Phoenix tag controller implementation; TWS=8, False LRU" << endl;
        PhoenixController controller(output_trace, output_log, 8, false);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-4t")) {
        cout << "Phoenix tag controller implementation; TWS=4, True LRU" << endl;
        PhoenixController controller(output_trace, output_log, 4, true);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-4f")) {
        cout << "Phoenix tag controller implementation; TWS=4, False LRU" << endl;
        PhoenixController controller(output_trace, output_log, 4, false);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-2t")) {
        cout << "Phoenix tag controller implementation; TWS=2, True LRU" << endl;
        PhoenixController controller(output_trace, output_log, 2, true);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else if (!strcmp(argv[1], "phoenix-2f")) {
        cout << "Phoenix tag controller implementation; TWS=2, False LRU" << endl;
        PhoenixController controller(output_trace, output_log, 2, false);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    } else {
        cout << "Baseline tag controller implementation" << endl;
        BaselineController controller(output_trace, output_log);
        controller.setup(decoder);
        count = Simulator::processTrace(decoder, controller, n, log_points);
        cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
        controller.reportStats();
    }

    cout << "Processed [" << count << "] entries" << endl;

    gzclose(trace);
    gzclose(output_trace);

    return 0;
}