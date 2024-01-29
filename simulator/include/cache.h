#pragma once

#include <cstdint>
#include <cassert>
#include <fstream>
#include <unordered_map>
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
    champsim_instr(uint64_t ip, uint64_t addr) : ip(ip), is_branch(0), branch_taken(0), destination_registers{}, source_registers{}, destination_memory{}, source_memory{} {
        //if (addr == 0) cout << "ADDR IS ZERO" << endl;
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
    int i;

public:
    explicit Cache(ofstream &output_trace) : data(), trace(std::move(output_trace)), i(0x10000) {
        //cout << sizeof(champsim_instr) << endl;
    }

    void logRead(uint64_t addr, uint16_t tags) {
        //cout << "CACHE READ  @ " << addr << " READ  " << tags << endl;
        champsim_instr trace_entry(i++, addr);
        trace.write((char *) &trace_entry, sizeof(trace_entry));
        //cout << i++ << " LOGGED: " << (int) trace_entry.ip << "#" << (int) trace_entry.is_branch << "#" << (int) trace_entry.branch_taken << "#" << (int) trace_entry.destination_registers[0] << ":" << (int) trace_entry.destination_registers[1] << "#" << (int) trace_entry.source_registers[0] << ":" << (int) trace_entry.source_registers[1] << ":" << (int) trace_entry.source_registers[2] << ":" << (int) trace_entry.source_registers[3] << "#" << (int) trace_entry.destination_memory[0] << ":" << (int) trace_entry.destination_memory[1] << "#" << (int) trace_entry.source_memory[0] << ":" << (int) trace_entry.source_memory[1] << ":" << (int) trace_entry.source_memory[2] << ":" << (int) trace_entry.source_memory[3] << endl;
        //if (i == 6180) assert(false);
    }

    void logWrite(uint64_t addr, uint16_t tags) {
        //cout << "CACHE WRITE @ " << addr << " WROTE " << tags << endl;
        // TODO: store data in dict when appropriate
        champsim_instr trace_entry(i++, addr);
        trace.write((char *) &trace_entry, sizeof(trace_entry));
        //cout << i++ << " LOGGED: " << (int) trace_entry.ip << "#" << (int) trace_entry.is_branch << "#" << (int) trace_entry.branch_taken << "#" << (int) trace_entry.destination_registers[0] << ":" << (int) trace_entry.destination_registers[1] << "#" << (int) trace_entry.source_registers[0] << ":" << (int) trace_entry.source_registers[1] << ":" << (int) trace_entry.source_registers[2] << ":" << (int) trace_entry.source_registers[3] << "#" << (int) trace_entry.destination_memory[0] << ":" << (int) trace_entry.destination_memory[1] << "#" << (int) trace_entry.source_memory[0] << ":" << (int) trace_entry.source_memory[1] << ":" << (int) trace_entry.source_memory[2] << ":" << (int) trace_entry.source_memory[3] << endl;
        //if (i == 6180) assert(false);
    }

    uint16_t doRead(uint64_t addr) {
        auto it = data.find(addr);
        assert(it != data.end());
        uint16_t tags = it->second;

        //cout << "CACHE DID DICTIONARY READ:" << endl;
        logRead(addr, tags);

        return tags;
    }
};
