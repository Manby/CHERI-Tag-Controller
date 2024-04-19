#pragma once

#include <cstdint>
#include <cassert>
#include <fstream>
#include <map>
#include <iostream>
#include <vector>
#include "schema.h"

using namespace std;

constexpr int BASE = 0x100000;

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
    map<uint64_t, Cacheline> data;
    ofstream trace;
    ofstream log;
    vector<champsim_instr> prevLogged;
    int i;

    static std::pair<uint64_t, uint16_t> addressToBaseOffsetPair(uint64_t addr) {  // converts an address into the base address of its cacheline and the offset into the cacheline

        uint64_t base = addr - (addr % TAG_CACHE_LINE_SIZE);   // base address of the cacheline (byte address)
        uint64_t offset = addr % TAG_CACHE_LINE_SIZE;   // offset into the cacheline (byte index)
        return {base, offset};
    }

public:
    explicit Cache(ofstream &output_trace, ofstream &output_log) : data(), trace(std::move(output_trace)), log(std::move(output_log)), i(BASE), prevLogged() {
        //cout << sizeof(champsim_instr) << endl;
    }

    void logRead(uint64_t addr) {
        DBG cout << "CACHE READ  @ " << addr << endl;
        champsim_instr trace_entry(i++, addressToBaseOffsetPair(addr).first);
        trace.write((char *) &trace_entry, sizeof(trace_entry));
        TST prevLogged.push_back(trace_entry);
        //cout << i++ << " LOGGED: " << (int) trace_entry.ip << "#" << (int) trace_entry.is_branch << "#" << (int) trace_entry.branch_taken << "#" << (int) trace_entry.destination_registers[0] << ":" << (int) trace_entry.destination_registers[1] << "#" << (int) trace_entry.source_registers[0] << ":" << (int) trace_entry.source_registers[1] << ":" << (int) trace_entry.source_registers[2] << ":" << (int) trace_entry.source_registers[3] << "#" << (int) trace_entry.destination_memory[0] << ":" << (int) trace_entry.destination_memory[1] << "#" << (int) trace_entry.source_memory[0] << ":" << (int) trace_entry.source_memory[1] << ":" << (int) trace_entry.source_memory[2] << ":" << (int) trace_entry.source_memory[3] << endl;
        //if (i == 6180) assert(false);
    }

    void logWrite(uint64_t addr) {
        DBG cout << "CACHE WRITE @ " << addr << endl;
        // TODO: store data in dict when appropriate
        champsim_instr trace_entry(i++, addressToBaseOffsetPair(addr).first);
        trace.write((char *) &trace_entry, sizeof(trace_entry));
        TST prevLogged.push_back(trace_entry);
        //cout << i++ << " LOGGED: " << (int) trace_entry.ip << "#" << (int) trace_entry.is_branch << "#" << (int) trace_entry.branch_taken << "#" << (int) trace_entry.destination_registers[0] << ":" << (int) trace_entry.destination_registers[1] << "#" << (int) trace_entry.source_registers[0] << ":" << (int) trace_entry.source_registers[1] << ":" << (int) trace_entry.source_registers[2] << ":" << (int) trace_entry.source_registers[3] << "#" << (int) trace_entry.destination_memory[0] << ":" << (int) trace_entry.destination_memory[1] << "#" << (int) trace_entry.source_memory[0] << ":" << (int) trace_entry.source_memory[1] << ":" << (int) trace_entry.source_memory[2] << ":" << (int) trace_entry.source_memory[3] << endl;
        //if (i == 6180) assert(false);
    }

    Cacheline doRead(uint64_t addr) {
        auto it = data.find(addr);
        assert(it != data.end());
        //if (it == data.end()) return {};
        Cacheline line = it->second;

        DBG cout << "CACHE DID DICTIONARY READ" << endl;
        logRead(addr);

        return line;
    }

    void doWrite(uint64_t addr, Cacheline line) {
        assert(addr % TAG_CACHE_LINE_SIZE == 0); // address should be a cacheline base address
        data[addr] = line;

        DBG cout << "CACHE DID DICTIONARY WRITE" << endl;
        logWrite(addr);
    }

    void set(uint64_t addr, Cacheline line) {
        assert(addr % TAG_CACHE_LINE_SIZE == 0); // address should be a cacheline base address
        data[addr] = line;
    }

    Cacheline peek(uint64_t addr) {
        auto it = data.find(addr);
        assert(it != data.end());
        //if (it == data.end()) return {};
        Cacheline line = it->second;

        DBG cout << "CACHE DID DICTIONARY PEEK" << endl;

        return line;
    }

    void dump() {
        cout << "DUMP: " << i-BASE << endl;
        Cacheline line;
        for (auto it = data.begin(); it != data.end(); it++) {        // do one cacheline line at a time
            line = it->second;
            log.write((char *) line.data(), TAG_CACHE_LINE_SIZE);
        }
    }

    int get_num_accesses() {
        return i-BASE;
    }

    vector<champsim_instr> getPrevLogged() {
        return prevLogged;
    }
};
