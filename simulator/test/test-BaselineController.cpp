#include <gtest/gtest.h>
#include "../include/trace.h"
#include "../include/BaselineController.h"
#include <iostream>

void checkAddrMapping(Controller &controller, access_type type, uint64_t addr, uint64_t target) {
    memAccess ax1{type, 64, 0b1101, addr};
    controller.handleMemoryAccess(ax1);
    champsim_instr logged = controller.getPrevLogged().back();
    EXPECT_EQ(logged.source_memory[0], target);
}

TEST(BaselineControllerTest, AddressMapping) {
    ifstream initial_accesses{"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin"};
    ifstream trace {"/media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests"};
    Decoder decoder(initial_accesses, trace);

    ofstream output_trace{"test_output_trace"};
    ofstream output_log{"test_output_log"};
    BaselineController controller(decoder, output_trace, output_log);

    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0x0000, 0);
    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0x0100, 0);

    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b01111111111111, 0);
    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b10000000000000, 64);
    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0b100000000000000, 128);

    checkAddrMapping(controller, ACCESS_TYPE_WRITE, 0xabcdcaf, 0xabcc0*2);


    checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0000, 0);
    checkAddrMapping(controller, ACCESS_TYPE_READ, 0x0100, 0);

    checkAddrMapping(controller, ACCESS_TYPE_READ, 0b01111111111111, 0);
    checkAddrMapping(controller, ACCESS_TYPE_READ, 0b10000000000000, 64);
    checkAddrMapping(controller, ACCESS_TYPE_READ, 0b100000000000000, 128);

    checkAddrMapping(controller, ACCESS_TYPE_READ, 0xabcdcaf, 0xabcc0*2);
}

/*
TEST(BaselineControllerTest, CacheStorage) {
    ofstream output_trace{"test_output_trace"};
    ofstream output_log{"test_output_log"};
    BaselineController controller(output_trace, output_log);

    memAccess ax1{ACCESS_TYPE_READ, 64, 0b0000, 0xabcdcafe};
    controller.handleMemoryAccess(ax1);
    champsim_instr logged = ;
    EXPECT_EQ(logged.source_memory[0], target);
}
*/