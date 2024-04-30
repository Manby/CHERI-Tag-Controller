#pragma once

#include <cstdint>
#include <cassert>
#include <fstream>
#include <map>
#include <iostream>
#include <vector>
#include "schema.h"
#include <zlib.h>

using namespace std;

constexpr int BASE = 0x100000;

// https://github.com/ChampSim/ChampSim/blob/b44625f3a2d4517b0bf80297f8819c797a966fe7/inc/trace_instruction.h#L35
constexpr size_t NUM_INSTR_DESTINATIONS = 2;
constexpr size_t NUM_INSTR_SOURCES = 4;

struct champsimInstr {
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
    champsimInstr(int64_t ip, uint64_t addr, bool isRead) : ip(ip), is_branch(0), branch_taken(0), destination_registers{}, source_registers{}, destination_memory{}, source_memory{} {
        //cout << addr << endl;
        if (addr == 0) addr = CSZERO; // to prevent champsim from ignoring 0x0 accesses
        if (isRead) source_memory[0] = addr;
        else destination_memory[0] = addr;
    }
};

class Memory {
private:
    map<uint64_t, Cacheline> data;
    gzFile trace;
    ofstream log;
    vector<champsimInstr> prevLogged;
    int i;

    static std::pair<uint64_t, uint16_t> addressToBaseOffsetPair(uint64_t addr) {  // converts an address into the base address of its cacheline and the offset into the cacheline

        uint64_t base = addr - (addr % TAG_CACHE_LINE_SIZE);   // base address of the cacheline (byte address)
        uint64_t offset = addr % TAG_CACHE_LINE_SIZE;   // offset into the cacheline (byte index)
        return {base, offset};
    }

    void logRead(uint64_t addr) {
        DBG cout << "CACHE READ  @ " << addr << endl;
        champsimInstr trace_entry(i++, addressToBaseOffsetPair(addr).first, true);
        gzwrite(trace, (char *) &trace_entry, sizeof(trace_entry));
        TST prevLogged.push_back(trace_entry);
    }

    void logWrite(uint64_t addr) {
        DBG cout << "CACHE WRITE @ " << addr << endl;
        // TODO: store data in dict when appropriate
        champsimInstr trace_entry(i++, addressToBaseOffsetPair(addr).first, false);
        gzwrite(trace, (char *) &trace_entry, sizeof(trace_entry));
        TST prevLogged.push_back(trace_entry);
    }

public:
    explicit Memory(gzFile output_trace, ofstream &output_log) : data(), trace(output_trace), log(std::move(output_log)), i(BASE), prevLogged() {
        //cout << sizeof(champsimInstr) << endl;
    }

    Cacheline doRead(uint64_t addr) {
        assert(addr % TAG_CACHE_LINE_SIZE == 0); // address should be a cacheline base address
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
        assert(addr % TAG_CACHE_LINE_SIZE == 0); // address should be a cacheline base address
        auto it = data.find(addr);
        assert(it != data.end());
        //if (it == data.end()) return {};
        Cacheline line = it->second;

        DBG cout << "CACHE DID DICTIONARY PEEK" << endl;

        return line;
    }

    void dump_table() {
        cout << "DUMP: " << i-BASE << endl;
        Cacheline line;
        for (auto & it : data) {        // do one cacheline line at a time
            line = it.second;
            log.write((char *) line.data(), TAG_CACHE_LINE_SIZE);
        }
    }

    int getNumAccesses() {
        return i-BASE;
    }

    vector<champsimInstr> getPrevLogged() {
        return prevLogged;
    }
};
