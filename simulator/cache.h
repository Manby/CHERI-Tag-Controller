//
// Created by kofi on 28/12/23.
//

#ifndef SIMULATOR_CACHE_H
#define SIMULATOR_CACHE_H

#include <cstdint>
#include <fstream>
#include "unordered_map"
using namespace std;

// TODO: cite?
// https://github.com/ChampSim/ChampSim/blob/b44625f3a2d4517b0bf80297f8819c797a966fe7/inc/trace_instruction.h#L35
constexpr size_t NUM_INSTR_DESTINATIONS = 2;
constexpr size_t NUM_INSTR_SOURCES = 4;

struct champsim_instr {
    // instruction pointer or PC (Program Counter)
    unsigned long long ip;

    // branch info
    unsigned char is_branch;
    unsigned char branch_taken;

    unsigned char destination_registers[NUM_INSTR_DESTINATIONS]; // output registers
    unsigned char source_registers[NUM_INSTR_SOURCES];           // input registers

    unsigned long long destination_memory[NUM_INSTR_DESTINATIONS]; // output memory
    unsigned long long source_memory[NUM_INSTR_SOURCES];           // input memory

    // TODO: revise
    champsim_instr(uint64_t addr) {
        source_memory[0] = addr;
    }
};

class Cache {

/* TODO: Should this class be inside Controller?
 * This way, I think we can have it so only the Controller class can access Cache's methods, which is nice hiding.
 */
private:
    unordered_map<uint64_t, bool> data;
    ofstream trace;

public:
    Cache(ofstream &output_trace);
    void logRead(uint64_t addr, uint16_t tags);
    void logWrite(uint64_t addr, uint16_t tags);
    uint16_t doRead(uint64_t addr);
};

#endif //SIMULATOR_CACHE_H
