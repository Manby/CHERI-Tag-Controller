/*
 * This class implements a controller for a standard, uncompressed tag table setup.
 */

#pragma once

#include <bitset>
#include <iostream>
#include "controller.h"

#define TAG_CACHE_LINE_SIZE 64      //size of the tag cache's cachelines in bytes (this should probably be in controller.h)

class BaselineController : public Controller {
private:
    std::pair<uint64_t, uint16_t> translateAddrDataToTag(uint64_t addrBase, uint8_t addrOffset) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow.....

        uint64_t tagBase = addrBase / (2 * TAG_CACHE_LINE_SIZE);   // base address of the tag cacheline (byte address)
        uint64_t tagOffset = 4 * (tagBase % (2 * TAG_CACHE_LINE_SIZE)) + addrOffset;   // offset into the cacheline (bit index)
        std::bitset<64> aB(addrBase);
        std::bitset<8> aO(addrOffset);
        std::bitset<64> tB(tagBase);
        std::bitset<16> tO(tagOffset);
        //cout << "Translation: " << aB << "," << aO << " became " << tB << "," << tO  << endl;
        return {tagBase, tagOffset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

public:
    BaselineController(ofstream &output_trace) : Controller(output_trace) {}; // TODO: can I avoid this line?

    void handleMemoryAccess(access ax) override {
        switch (ax.type) {
            case ACCESS_TYPE_READ:
                cache.logRead(translateAddrDataToTag(ax.addr, 0).first, ax.tags);
                break;
            case ACCESS_TYPE_WRITE:
                cache.logWrite(translateAddrDataToTag(ax.addr, 0).first, ax.tags);
        }
    }
};