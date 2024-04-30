#include "../include/Decoder.h"
#include "../include/Simulator.h"
#include "../include/BaselineController.h"
#include "../include/ETMController.h"
#include "../include/MorelloController.h"
#include "../include/PhoenixController.h"
#include "../include/DummyOneController.h"
#include "../include/DummyZeroController.h"
#include <fstream>
#include <vector>
#include <unordered_set>
#include <string.h>

int verify(Decoder &decoder, Controller &controller, size_t n) {
    uint16_t tags, bufferTags;
    size_t i = 0;
    int discrepancies = 0;

    vector<memAccess> buffer(CHUNK_SIZE);
    for (size_t at = 0; at < n; at += CHUNK_SIZE) {
        size_t read = decoder.readLLCMisses(buffer, min(CHUNK_SIZE, n-at));

        int i = 0;
        for (size_t remaining = read; remaining > 0; --remaining) {     // CLion hint can be ignored
            if (buffer[i].type == ACCESS_TYPE_READ) {
                tags = controller.handleRead(buffer[i]);  // calls are automatically inlined
                bufferTags = buffer[i].tags;
                if (tags != bufferTags) {
                    cout << "TAGS DIFFER AT INSTRUCTION INDEX " << i << endl;
                    cout << "ADDRESS:       " << buffer[i].addr << endl;
                    cout << "TRACE TAGS:    " << bufferTags << endl;
                    cout << "BASELINE TAGS: " << tags << endl;
                    discrepancies++;
                }

            } else controller.handleWrite(buffer[i]);

            ++i;
        }
    }

    cout << "VERIFY: " << discrepancies << " total discrepancies" << endl;

    return discrepancies;
}

int crossVerify(BaselineController &baselineController, Controller &controller) {
    memAccess ax;
    uint16_t baselineTags, tags;
    int discrepancies = 0;

    for (uint64_t addr = 0; addr < MEMORY_SIZE; addr += 16) {
        if ((addr << 4) % (MEMORY_SIZE) == 0) cout << "CROSS-VERIFY: " << ((addr << 4) / (MEMORY_SIZE)) << "/16 complete" << endl;

        ax = memAccess(ACCESS_TYPE_READ, 64, 0, addr);

        baselineTags = baselineController.handleRead(ax);
        tags = controller.handleRead(ax);

        if (tags != baselineTags) {
            cout << "TAGS DIFFER AT ADDRESS " << addr << endl;
            cout << "BASELINE TAGS: " << baselineTags << endl;
            cout << "OTHER TAGS:    " << tags << endl;
            discrepancies++;
        }
    }

    cout << "CROSS-VERIFY: " << discrepancies << " total discrepancies" << endl;

    return discrepancies;
}

int main(int argc, char *argv[]) {
    cout << "CHERI TAG CONTROLLER VERIFIER" << endl;

    if (argc < 4) {
        cout << "Not enough arguments given" << endl;
        return 1;
    }

    ifstream initial_accesses {argv[2]};
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
    if (argc >= 5) {
        stringstream str(argv[4]);
        str >> n;
    } else {
        n = 5000000;
    }

    size_t baselineCount, count;

    gzFile output_trace = gzopen("/dev/null", "wb");
    ofstream output_log{"/dev/null"};
    unordered_set<size_t> log_points{};

    if (strcmp(argv[1], "baseline") != 0) {
        // Cross-verify the specified controller against the Baseline controller

        cout << "Simulating Baseline" << endl;
        BaselineController baselineController(output_trace, output_log);
        baselineController.setup(decoder);
        baselineCount = Simulator::processTrace(decoder, baselineController, n, log_points);
        cout << "Processed [" << baselineCount << "] entries" << endl;

        if (!strcmp(argv[1], "morello-t")) {
            cout << "Simulating Morello tag controller implementation, Tag Cache" << endl;
            MorelloController controller(output_trace, output_log, true);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else if (!strcmp(argv[1], "morello-z")) {
            cout << "Simulating Morello tag controller implementation, Zero Cache" << endl;
            MorelloController controller(output_trace, output_log, false);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else if (!strcmp(argv[1], "etm")) {
            cout << "Simulating ETM tag controller implementation" << endl;
            ETMController controller(output_trace, output_log);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else if (!strcmp(argv[1], "phoenix")) {
            cout << "Simulating Phoenix tag controller implementation" << endl;
            PhoenixController controller(output_trace, output_log, 8);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else if (!strcmp(argv[1], "dummyzero")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            DummyZeroController controller(output_trace, output_log);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else if (!strcmp(argv[1], "dummyone")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            DummyOneController controller(output_trace, output_log);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        } else {
            cout << "Baseline tag controller implementation" << endl;
            BaselineController controller(output_trace, output_log);
            controller.setup(decoder);
            count = Simulator::processTrace(decoder, controller, n, log_points);
            cout << "Performed [" << controller.getNumAccesses() << "] accesses" << endl;
            controller.reportStats();

            cout << "Processed [" << count << "] entries" << endl;
            assert(count == baselineCount);
            crossVerify(baselineController, controller);
        }
    } else {
        // verify the Baseline controller
        cout << "Baseline tag controller implementation" << endl;
        BaselineController controller(output_trace, output_log);
        controller.setup(decoder);

        cout << "Beginning verification" << endl;
        verify(decoder, controller, n);
    }

    gzclose(trace);
    gzclose(output_trace);

    return 0;
}
