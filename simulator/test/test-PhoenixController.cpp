#include <gtest/gtest.h>
#include "../include/trace.h"
#include "../include/PhoenixController.h"
#include <iostream>

uint16_t doRead(Controller &controller, uint64_t addr) {
    memAccess ax1{ACCESS_TYPE_READ, 64, 0b0000, addr};
    return controller.handleRead(ax1);  // calls are automatically inlined
}

void doWrite(Controller &controller, uint64_t addr, uint16_t tags) {
    memAccess ax1{ACCESS_TYPE_WRITE, 64, tags, addr};
    controller.handleWrite(ax1);  // calls are automatically inlined
}

TEST(PhoenixControllerTest, DRAMUsage) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    gzFile trace = gzopen("/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests", "rb");
    Decoder decoder(initial_accesses, trace);

    gzFile output_trace = gzopen("test_output_trace.gz", "wb");
    ofstream output_log{"test_output_log"};
    PhoenixController controller(output_trace, output_log, 8, true);
    controller.setup(decoder);

    controller.reportStats();

    doWrite(controller, 0b00000000000000000000000000, 0b1111);   // write to already non-zero block
    controller.reportStats();

    doWrite(controller, 0b10000000000000000000000000, 0b1111);    // write to zero block
    controller.reportStats();   // expect the maximum number of blocks to increase by one

    //EXPECT_EQ(0, 1);

    gzclose(trace);
    gzclose(output_trace);
}