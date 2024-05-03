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

int crossVerify(Controller &controller1, Controller &controller2) {
    memAccess ax;
    uint16_t baselineTags, tags;
    int discrepancies = 0;

    for (uint64_t addr = 0; addr < MEMORY_SIZE; addr += 16) {
        if ((addr << 4) % (MEMORY_SIZE) == 0) cout << "CROSS-VERIFY: " << ((addr << 4) / (MEMORY_SIZE)) << "/16 complete" << endl;

        ax = memAccess(ACCESS_TYPE_READ, 64, 0, addr);

        baselineTags = controller1.handleRead(ax);
        tags = controller2.handleRead(ax);

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

    ifstream initial_accesses {argv[1]};
    gzFile trace = gzopen(argv[2], "rb");

    if (!initial_accesses) {
        cout << "No file found by the name " << argv[1] << endl;
        return 2;
    }

    if (!trace) {
        cout << "No file found by the name " << argv[2] << endl;
        return 3;
    }

    Decoder decoder = Decoder(initial_accesses, trace);

    size_t n;
    stringstream str(argv[3]);
    str >> n;

    gzFile output_trace = gzopen("/dev/null", "wb");
    ofstream output_log{"/dev/null"};
    unordered_set<size_t> log_points{};
    if (argc >= 6) {
        // cross-verify
        size_t count1, count2;
        Controller *controller1p;
        Controller *controller2p;

        // simulation 1
        if (!strcmp(argv[4], "morello-t")) {
            cout << "Simulating Morello tag controller implementation, Tag Cache" << endl;
            controller1p = new MorelloController(output_trace, output_log, true);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else if (!strcmp(argv[4], "morello-z")) {
            cout << "Simulating Morello tag controller implementation, Zero Cache" << endl;
            controller1p = new MorelloController (output_trace, output_log, false);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else if (!strcmp(argv[4], "etm")) {
            cout << "Simulating ETM tag controller implementation" << endl;
            controller1p = new ETMController (output_trace, output_log);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else if (!strcmp(argv[4], "phoenix")) {
            cout << "Simulating Phoenix tag controller implementation" << endl;
            controller1p = new PhoenixController (output_trace, output_log, 8, true);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else if (!strcmp(argv[4], "dummyzero")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller1p = new DummyZeroController (output_trace, output_log);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else if (!strcmp(argv[4], "dummyone")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller1p = new DummyOneController (output_trace, output_log);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        } else {
            cout << "Simulating Baseline tag controller implementation" << endl;
            controller1p = new BaselineController (output_trace, output_log);
            controller1p->setup(decoder);
            count1 = Simulator::processTrace(decoder, *controller1p, n, log_points);
            cout << "Performed [" << controller1p->getNumAccesses() << "] accesses" << endl;
            controller1p->reportStats();

            cout << "Processed [" << count1 << "] entries" << endl;
        }

        // simulation 2
        if (!strcmp(argv[5], "morello-t")) {
            cout << "Simulating Morello tag controller implementation, Tag Cache" << endl;
            controller2p = new MorelloController (output_trace, output_log, true);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else if (!strcmp(argv[5], "morello-z")) {
            cout << "Simulating Morello tag controller implementation, Zero Cache" << endl;
            controller2p = new MorelloController (output_trace, output_log, false);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else if (!strcmp(argv[5], "etm")) {
            cout << "Simulating ETM tag controller implementation" << endl;
            controller2p = new ETMController (output_trace, output_log);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else if (!strcmp(argv[5], "phoenix")) {
            cout << "Simulating Phoenix tag controller implementation" << endl;
            controller2p = new PhoenixController (output_trace, output_log, 8, true);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else if (!strcmp(argv[5], "dummyzero")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller2p = new DummyZeroController (output_trace, output_log);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else if (!strcmp(argv[5], "dummyone")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller2p = new DummyOneController (output_trace, output_log);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        } else {
            cout << "Simulating Baseline tag controller implementation" << endl;
            controller2p = new BaselineController (output_trace, output_log);
            controller2p->setup(decoder);
            count2 = Simulator::processTrace(decoder, *controller2p, n, log_points);
            cout << "Performed [" << controller2p->getNumAccesses() << "] accesses" << endl;
            controller2p->reportStats();

            cout << "Processed [" << count2 << "] entries" << endl;
        }

        assert(count1 == count2);
        crossVerify(*controller1p, *controller2p);

        delete controller1p;
        delete controller2p;

    } else {
        // verify
        size_t count1;
        Controller *controller1p;

        // simulation 1
        if (!strcmp(argv[4], "morello-t")) {
            cout << "Simulating Morello tag controller implementation, Tag Cache" << endl;
            controller1p = new MorelloController(output_trace, output_log, true);
            controller1p->setup(decoder);
        } else if (!strcmp(argv[4], "morello-z")) {
            cout << "Simulating Morello tag controller implementation, Zero Cache" << endl;
            controller1p = new MorelloController (output_trace, output_log, false);
            controller1p->setup(decoder);
        } else if (!strcmp(argv[4], "etm")) {
            cout << "Simulating ETM tag controller implementation" << endl;
            controller1p = new ETMController (output_trace, output_log);
            controller1p->setup(decoder);
        } else if (!strcmp(argv[4], "phoenix")) {
            cout << "Simulating Phoenix tag controller implementation" << endl;
            controller1p = new PhoenixController (output_trace, output_log, 8, true);
            controller1p->setup(decoder);
        } else if (!strcmp(argv[4], "dummyzero")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller1p = new DummyZeroController (output_trace, output_log);
            controller1p->setup(decoder);
        } else if (!strcmp(argv[4], "dummyone")) {
            cout << "Simulating Dummy Zero tag controller implementation" << endl;
            controller1p = new DummyOneController (output_trace, output_log);
            controller1p->setup(decoder);
        } else {
            cout << "Simulating Baseline tag controller implementation" << endl;
            controller1p = new BaselineController (output_trace, output_log);
            controller1p->setup(decoder);
        }

        verify(decoder, *controller1p, n);

        delete controller1p;
    }

    gzclose(trace);
    gzclose(output_trace);

    return 0;
}
