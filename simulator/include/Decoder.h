#pragma once

#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include "trace.h"
#include "schema.h"
#include <cassert>

using namespace std;

class Decoder {
public:
    ifstream initial_accesses;
private:
    ifstream trace;

    bool getInitialTag(uint64_t index) {
        assert(index < 134217728); // 2^27 bytes

        // TODO: 2 proper casts
        initial_accesses.seekg(index);  // TODO: explicit conversion (excon)

        uint8_t intermediate;
        //cout << "reading out initial access data at index " << index << endl;
        initial_accesses.read((char *) &intermediate, 1);

        size_t numRead = initial_accesses.gcount();
        assert(numRead == 1);

        bool tag = intermediate >> 7;
        int8_t type = intermediate & 0x7f;  // TODO: excon

        return assumeTag(tag, type);
    }

    memAccess furnish(llcMiss miss) {
        // TODO: refactor assert (?; and change == 15 check if necessary)
        assert(miss.size == 64);
        assert(miss.addr % 64 == 0);    // should be a 64-byte aligned address, as each cacheline is 64 bytes long
        //if (miss.type == LLC_MISS_TYPE_WRITE && miss.tags != 0 && miss.tags_known != 15) cout << "WRITE TAGS " << miss.tags << "; KNOWN " << miss.tags_known << (miss.tags_known == 15 ? "" : "!!!") << endl;

        if (miss.tags_known != 15) {
            int i = 0;
            //cout << "this miss has unknown tags: " << miss.tags_known << endl;
            for (uint16_t base = 16; base >= 1; base >>= 1) {   // left-to-right; MSB-to-LSB
                if (!(miss.tags_known & base)) {
                    //cout << "need to look up tag " << i << " for cacheline base address " << miss.addr << endl;
                    uint64_t index = (miss.addr / 16) + i;
                    if (getInitialTag(index)) {
                        miss.tags |= base;      // set the tag
                    } else {
                        miss.tags &= ~base;     // clear the tag
                    }
                }
                ++i;
            }
        }

        return memAccess(
                miss.type == LLC_MISS_TYPE_READ ? ACCESS_TYPE_READ : ACCESS_TYPE_WRITE,
                miss.size,
                miss.tags,
                miss.addr
        );
    }

public:
    Decoder(ifstream &initial_accesses, ifstream &trace) : initial_accesses(std::move(initial_accesses)), trace(std::move(trace)) {}

    /*
    size_t Decoder::read_init_accesses(initialAccess *buffer, size_t n) {
        uint8_t intermediate[n];
        // TODO: could the reading from the file and the struct conversion happen in parallel? i.e. multithreading
        size_t numRead = fread(intermediate, 1, n, initial_accesses);

        for (int i = 0; i < numRead; ++i) {
            bool tag = intermediate[i] >> 7;
            int8_t type = intermediate[i] & 0x7f;
            buffer[i] = initialAccess(type, tag);
        }

        return numRead;
    }
    */

    size_t read_llc_misses(vector<memAccess> &buffer, size_t n) {
        auto intermediate = new vector<uint8_t>(16*buffer.size());  // llcMiss is 16 bytes
        // TODO: could the reading from the file and the struct conversion happen in parallel? i.e. multithreading
        trace.read((char *) intermediate->data(), n*16); // TODO: explicit conversion

        size_t bytesRead = trace.gcount();
        for (int i = 0; i < bytesRead; i += 16) {
            auto type = (llcMissType) intermediate->at(i);
            uint16_t size = intermediate->at(i+2) + ((uint16_t) intermediate->at(i+3) << 8);
            uint16_t tags = intermediate->at(i+4) + ((uint16_t) intermediate->at(i+5) << 8);
            uint16_t tags_known = intermediate->at(i+6) + ((uint16_t) intermediate->at(i+7) << 8);
            uint64_t addr = 0;
            for (int j = 0; j < 8; ++j) {
                addr += ((uint64_t) intermediate->at(i+8+j)) << (8*j);
            }
            addr -= QEMU_BASE_ADDRESS;

            llcMiss miss = llcMiss(type, size, tags, tags_known, addr);

            buffer[i/16] = furnish(miss);   // convert the llcMiss into an access by furnishing it
        }

        return bytesRead/16;    // return number of llcMiss structs read
    }


    template<size_t n> void getTags(uint64_t base_index, int num_tags, array<uint8_t, n> &buffer) {
        assert(num_tags % 8 == 0);      // should be a multiple of 8
        uint8_t intermediate[num_tags];
        initial_accesses.seekg(base_index);
        initial_accesses.read((char *) &intermediate,num_tags);   // read out a cacheline's worth of initial_access structs
        size_t numRead = initial_accesses.gcount();
        assert(numRead == num_tags);

        for (int i = 0; i < num_tags/8; ++i) {
            uint8_t byte = 0;
            for (int t = 0; t < 8; ++t) {
                byte <<= 1;
                bool tag = intermediate[8*i + t] >> 7;
                int8_t type = intermediate[8*i + t] & 0x7f;  // TODO: excon
                byte |= assumeTag(tag, type) ? 1 : 0;
            }
            buffer[i] = byte;
        }
    }
};