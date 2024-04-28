#include <gtest/gtest.h>
#include "../include/trace.h"
#include "../include/ETMController.h"
#include <iostream>

uint64_t checkAddrMapping(Controller &controller, access_type type, uint64_t addr) {
    memAccess ax1{type, 64, 0b1101, addr};
    if (ax1.type == ACCESS_TYPE_READ) controller.handleRead(ax1);  // calls are automatically inlined
    else controller.handleWrite(ax1);
    champsimInstr logged = controller.getPrevLogged().back();
    return logged.source_memory[0];
}

// Root: //128 //512 (== <<7 <<9)
TEST(ETMControllerTest, AddressMappingAndReadAccessSequence) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    ifstream trace {"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests"};
    Decoder decoder(initial_accesses, trace);

    ofstream output_trace{"test_output_trace"};
    ofstream output_log{"test_output_log"};
    ETMController controller(output_trace, output_log);

    uint64_t result;
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0000);
    EXPECT_EQ(result, 0);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0100);
    EXPECT_EQ(result, 0);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b01111111000000);
    EXPECT_EQ(result, 0);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b10000000000000);
    EXPECT_EQ(result, 0);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b100000000000000); // a non-zero line
    vector<champsimInstr> logged = controller.getPrevLogged();
    EXPECT_EQ(logged.at(logged.size()-2).source_memory[0], 0);
    EXPECT_EQ(logged.at(logged.size()-1).source_memory[0], ROOT_TABLE_SIZE+128);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b01111111111111111000000);
    EXPECT_EQ(result, 0);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b10000000000000000000000);
    EXPECT_EQ(result, 64);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b1000000000000000000000000000);
    EXPECT_EQ(result, 2048);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0xabcdc00);
    EXPECT_EQ(result, 0b101010000000);
}

TEST(ETMControllerTest, WriteAccessSequence) {
    // root 0 write 0: read root

    // root 0 write still 1: read root write root read leaf write leaf

    // root 1 write now 0: read root read leaf write leaf write root

    // root 1 write 1: read root read leaf write leaf
}

/*
TEST(BaselineControllerTest, CacheStorage) {
    ofstream output_trace{"test_output_trace"};
    ofstream output_log{"test_output_log"};
    BaselineController controller(output_trace, output_log);

    memAccess ax1{ACCESS_TYPE_READ, 64, 0b0000, 0xabcdcafe};
    controller.handleMemoryAccess(ax1);
    champsimInstr logged = ;
    EXPECT_EQ(logged.source_memory[0], target);
}
*/