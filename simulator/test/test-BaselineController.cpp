#include <gtest/gtest.h>
#include "../include/trace.h"
#include "../include/BaselineController.h"
#include <iostream>

uint64_t checkAddrMapping(Controller &controller, access_type type, uint64_t addr) {
    memAccess ax1{type, 64, 0b1101, addr};
    if (ax1.type == ACCESS_TYPE_READ) controller.handleRead(ax1);  // calls are automatically inlined
    else controller.handleWrite(ax1);
    champsimInstr logged = controller.getPrevLogged().back();
    if (ax1.type == ACCESS_TYPE_READ) return logged.source_memory[0];
    else return logged.destination_memory[0];
}

TEST(BaselineControllerTest, AddressMapping) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    gzFile trace = gzopen("/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests", "rb");
    Decoder decoder(initial_accesses, trace);

    gzFile output_trace = gzopen("test_output_trace.gz", "wb");
    ofstream output_log{"test_output_log"};
    BaselineController controller(output_trace, output_log);
    controller.setup(decoder);

    uint64_t result;
    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0x0000);
    EXPECT_EQ(result, CSZERO);

    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0x0100);
    EXPECT_EQ(result, CSZERO);

    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b01111111111111);
    EXPECT_EQ(result, CSZERO);
    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b10000000000000);
    EXPECT_EQ(result, 64);
    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b100000000000000);
    EXPECT_EQ(result, 128);

    result = checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0xabcdcaf);
    EXPECT_EQ(result, 0xabcc0*2);


    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0000);
    EXPECT_EQ(result, CSZERO);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0100);
    EXPECT_EQ(result, CSZERO);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b01111111111111);
    EXPECT_EQ(result, CSZERO);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b10000000000000);
    EXPECT_EQ(result, 64);
    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0b100000000000000);
    EXPECT_EQ(result, 128);

    result = checkAddrMapping(controller, ACCESS_TYPE_READ, 0xabcdcaf);
    EXPECT_EQ(result, 0xabcc0*2);

    gzclose(trace);
    gzclose(output_trace);
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