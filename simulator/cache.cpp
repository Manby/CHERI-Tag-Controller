//
// Created by kofi on 28/12/23.
//

#include "cache.h"
#include "unordered_map"
#include <iostream>
#include <cassert>

Cache::Cache(ofstream &output_trace) : data(), trace(std::move(output_trace)) {}

void Cache::logRead(uint64_t addr, uint16_t tags) {
    cout << "CACHE READ  @ " << addr << " READ  " << tags << endl;
    champsim_instr trace_entry(addr);
    trace.write((char *) &trace_entry, sizeof(trace_entry));
}

void Cache::logWrite(uint64_t addr, uint16_t tags) {
    cout << "CACHE WRITE @ " << addr << " WROTE " << tags << endl;
    // TODO: store data in dict when appropriate
    champsim_instr trace_entry(addr);
    trace.write((char *) &trace_entry, sizeof(trace_entry));
}

uint16_t Cache::doRead(uint64_t addr) {
    auto it = data.find(addr);
    assert(it != data.end());
    uint16_t tags = it->second;

    cout << "CACHE DID DICTIONARY READ:" << endl;
    logRead(addr, tags);

    return tags;
}
