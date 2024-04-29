#include <gtest/gtest.h>
#include "../include/trace.h"
#include "../include/MorelloController.h"
#include <iostream>

TEST(MorelloControllerZeroTest, ReadAccessSequence) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    gzFile trace = gzopen("/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests", "rb");
    Decoder decoder(initial_accesses, trace);

    gzFile output_trace = gzopen("test_output_trace.gz", "wb");
    ofstream output_log{"test_output_log"};

    MorelloController controller(output_trace, output_log, false);

    // zero cache should handle this one
    memAccess ax1{ACCESS_TYPE_READ, 64, 0b0000, 0xabcdcaf};
    controller.handleRead(ax1);
    vector<champsimInstr> logged = controller.getPrevLogged();
    ASSERT_EQ(logged.size(), 1);
    EXPECT_EQ(logged.back().source_memory[0], 0xabcc0*2);

    // tag cache should handle this one
    memAccess ax2{ACCESS_TYPE_READ, 64, 0b0000, 0x100*128};
    controller.handleRead(ax2);
    logged = controller.getPrevLogged();
    EXPECT_EQ(logged.size(), 1);
}

/*
TEST(MorelloControllerZeroTest, WriteAccessSequence) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    ifstream trace {"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests"};
    Decoder decoder(initial_accesses, trace);

    ofstream output_trace{"test_output_trace"};
    ofstream output_log{"test_output_log"};

    MorelloController controller(decoder, output_trace, output_log, false);

    // zero cache should handle this one
    memAccess ax1{ACCESS_TYPE_WRITE, 64, 0b0000, 0xabcdcaf};
    controller.handleMemoryAccess(ax1);
    vector<champsimInstr> logged = controller.getPrevLogged();
    ASSERT_EQ(logged.size(), 1);
    EXPECT_EQ(logged.back().source_memory[0], 0xabcc0*2);

    // tag cache should handle this one
    memAccess ax2{ACCESS_TYPE_READ, 64, 0b0000, 0x100*128};
    controller.handleMemoryAccess(ax2);
    logged = controller.getPrevLogged();
    EXPECT_EQ(logged.size(), 1);
}
*/


// Duplicate above for tag cache