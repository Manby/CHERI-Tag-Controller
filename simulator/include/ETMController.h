/*
 * This class implements the controller described in the Efficient Tagged Memory Paper.
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8119285&tag=1
 */

#pragma once

#include <iostream>
#include "Controller.h"
#include "Decoder.h"
#include "schema.h"
#include "utils.h"

class ETMController : public Controller {
protected:
    static std::pair<uint64_t, uint16_t> translateToLeafAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t leaf_base = (((addr_base / 128) >> 6) << 6) + LEAF_TABLE_BASE;  // base address of the tag cacheline (byte address)
        uint16_t leaf_offset = ((addr_base>>6)*4) % 512;         // index into the tag cacheline (bit-index; multiple of 4 in [0,512))
        return {leaf_base, leaf_offset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

    static std::pair<uint64_t, uint16_t> translateToRootAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t root_base = ((addr_base / 65536) >> 6) << 6;  // base address of the tag cacheline (byte address)
        uint16_t root_offset = ((addr_base>>6)*4 / 512) % 512;         // index into the tag cacheline (bit-index; in [0,512))
        return {root_base, root_offset};
    }

public:
    ETMController(ofstream &output_trace, ofstream &output_log) : Controller(output_trace, output_log) {}

    void setupCache(Decoder &decoder) override {
        Cacheline root_line, leaf_line;
        uint8_t root_byte;
        bool no_leaves_set;

        for (uint64_t rl = 0; rl < LEAF_TABLE_BASE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                root_byte = 0;
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags
                    root_byte <<= 1;

                    // focus on this single root tag
                    decoder.getInitialTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    no_leaves_set = isClear(leaf_line);

                    cache.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) + LEAF_TABLE_BASE, leaf_line);     // insert the leaf line into the cache

                    root_byte |= no_leaves_set ? 0 : 1;
                }

                root_line[rb] = root_byte;      // push the root byte into the line
            }

            cache.set(rl, root_line);     // insert the root line into the cache
        }
    };

    void handleMemoryAccess(memAccess ax) override {
        uint64_t root_base_addr, root_cacheline_index, leaf_base_addr, leaf_cacheline_index;
        Cacheline root_line, leaf_line;
        pair<uint64_t, uint16_t> result;
        bool root_tag;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                result = translateToRootAddr(ax.addr);
                root_base_addr = result.first;
                root_cacheline_index = result.second;
                root_line = cache.doRead(root_base_addr);

                root_tag = getTag(root_line, root_cacheline_index);
                // TODO: Check that the above line isn't doing it backwards!

                if (!root_tag) {
                    DBG cout << "Root tag was zero; short circuit!" << endl;
                    return;   // we would return a 0 to the client
                }
                TST cout << "READING A NON-ZERO LINE at " << ax.addr << endl;

                leaf_base_addr = translateToLeafAddr(ax.addr).first;
                cache.doRead(leaf_base_addr);

                return; // we would return whatever the leaf is

            case ACCESS_TYPE_WRITE:
                if (ax.tags != 0) {
                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    root_line = cache.doRead(root_base_addr);

                    root_tag = getTag(root_line, root_cacheline_index);

                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;
                    if (!root_tag) {
                        DBG cout << "Setting the root (it was cleared)" << endl;
                        modifyTag(root_line, root_cacheline_index, 1);
                        cache.doWrite(root_base_addr, root_line);

                        leaf_line = Cacheline{};    // a fully-zero cacheline
                    } else {
                        leaf_line = cache.doRead(leaf_base_addr);
                    }
                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);
                    cache.doWrite(leaf_base_addr, leaf_line);

                } else {
                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;
                    leaf_line = cache.doRead(leaf_base_addr);

                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);

                    if (!isClear(leaf_line)) {
                        cache.doWrite(leaf_base_addr, leaf_line);
                        return;
                    }

                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    root_line = cache.doRead(root_base_addr);

                    modifyTag(root_line, root_cacheline_index, 0);
                    cache.doWrite(root_base_addr, root_line);
                }
        }
    }
};