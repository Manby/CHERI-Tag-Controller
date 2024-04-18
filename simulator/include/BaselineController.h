/*
 * This class implements a controller for a standard, uncompressed tag table setup.
 */

#pragma once

#include <bitset>
#include <iostream>
#include "controller.h"
#include "schema.h"
#include "decoder.h"

#define TAG_CACHE_LINE_SIZE 64      //size of the tag cache's cachelines in bytes (this should probably be in controller.h)

class BaselineController : public Controller {
private:
    static std::pair<uint64_t, uint16_t> translateAddrDataToTag(uint64_t addrBase,
                                                                uint8_t addrOffset) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow.....

        uint64_t tagBase = addrBase / (2 * TAG_CACHE_LINE_SIZE);   // base address of the tag cacheline (byte address)
        uint64_t tagOffset =
                4 * (tagBase % (2 * TAG_CACHE_LINE_SIZE)) + addrOffset;   // offset into the cacheline (bit index)
        //std::bitset<64> aB(addrBase);
        //std::bitset<8> aO(addrOffset);
        //std::bitset<64> tB(tagBase);
        //std::bitset<16> tO(tagOffset);
        //cout << "Translation: " << aB << "," << aO << " became " << tB << "," << tO  << endl;
        return {tagBase, tagOffset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

public:
    BaselineController(Decoder &decoder, ofstream &output_trace, ofstream &output_log) : Controller(output_trace, output_log) {
        array<uint8_t, TAG_CACHE_LINE_SIZE> root_line, leaf_line;

        for (uint64_t rl = 0; rl < LEAF_TABLE_BASE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags

                    // focus on this single root tag
                    decoder.getTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    /*
                    for (uint8_t byte : leaf_line) {
                        if (byte) {
                            cout << (8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8 << "!!!" << endl;
                        }
                    }
                    */

                    //cout << "SETTING LEAF @ " << TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) << endl;
                    cache.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt), leaf_line);     // insert the leaf line into the cache
                }
            }
        }
    }

    void handleMemoryAccess(memAccess ax) override {
        switch (ax.type) {
            case ACCESS_TYPE_READ:
                cache.logRead(translateAddrDataToTag(ax.addr, 0).first);
                break;
            case ACCESS_TYPE_WRITE:
                cache.logWrite(translateAddrDataToTag(ax.addr, 0).first);
        }
    }
};