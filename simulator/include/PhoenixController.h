/*
 * This class implements the tag controller described in my Phoenix scheme.
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8119285&tag=1
 */

#pragma once

#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include "Controller.h"
#include "Decoder.h"
#include "schema.h"
#include "utils.h"

constexpr uint64_t SUPERROOT_TABLE_SIZE = ROOT_TABLE_SIZE >> 9;

class PhoenixController : public Controller {
protected:
    vector<uint16_t> tag_working_set;    // set of blocks in the current tag working set; ordered by LRU policy, with LRU at the front
    unordered_set<uint16_t> allocated;          // set of blocks that currently have an allocated portion of DRAM
    uint16_t twsSize;                           // maximum size of the tag working set
    bool useTrueLRU;                            // whether we are using true LRU or an approximation of it
    unordered_map<uint16_t, int> dramUsage;
    int allocCount, freeCount;

    static std::pair<uint64_t, uint16_t> translateToLeafAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t leaf_base = (((addr_base / 128) >> 6) << 6) + SUPERROOT_TABLE_SIZE + ROOT_TABLE_SIZE;  // base address of the tag cacheline (byte address)
        uint16_t leaf_offset = ((addr_base>>6)*4) % 512;         // index into the tag cacheline (bit-index; multiple of 4 in [0,512))
        //cout << "LEAF ADDR " << leaf_base << endl;
        return {leaf_base, leaf_offset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

    static std::pair<uint64_t, uint16_t> translateToRootAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t root_base = (((addr_base / 65536) >> 6) << 6) + SUPERROOT_TABLE_SIZE;  // base address of the tag cacheline (byte address)
        uint16_t root_offset = ((addr_base>>6)*4 / 512) % 512;         // index into the tag cacheline (bit-index; in [0,512))
        //cout << "ROOT ADDR " << root_base << endl;
        return {root_base, root_offset};
    }

    static std::pair<uint64_t, uint16_t> translateToSuperrootAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t superroot_base = ((addr_base / 33554432) >> 6) << 6;  // base address of the tag cacheline (byte address) -- this will always be 0 when memory is 2GiB (1 << 31)
        uint16_t superroot_offset = ((addr_base >> 6) * 4 / 512 / 512) % 512;         // index into the tag cacheline (bit-index; in [0,512))
        //cout << "SPRT ADDR " << superroot_base << endl;
        return {superroot_base, superroot_offset};
    }

    uint16_t blockIndex(uint64_t addr) {
        return (addr / 65536) >> 6;     // leaves only the top 9 bits of the addr (so its in the range [0,512))
    }

    bool isBlockClear(uint16_t index) {
        Cacheline line = memory.doRead(0);
        return !getTag(line, index);
    }

    bool isInTWS(uint16_t index) {
        return find(tag_working_set.begin(), tag_working_set.end(), index) != tag_working_set.end();
    }

    void promote(uint16_t index) {
        auto it = find(tag_working_set.begin(), tag_working_set.end(), index);
        assert(it != tag_working_set.end());
        tag_working_set.erase(it);
        tag_working_set.push_back(index);
    }

    uint16_t updateTWS(uint16_t index) {           // returns the index of the evicted block
        if (isInTWS(index)) {
            promote(index);
            return 0xffff;
        }

        tag_working_set.push_back(index);

        if (tag_working_set.size() > twsSize) {
            auto it = tag_working_set.begin();
            if (!useTrueLRU) for (int i = 2; i < twsSize; i++) it++;  // if approximating LRU, evict the second-most-recently used, to provide a worst-case bound
            uint16_t evicted = *it;
            tag_working_set.erase(it);     // evict victim

            if (isBlockClear(evicted)) DRAMFree(evicted);

            return evicted;
        }

        return 0xffff;
    }

    void DRAMAllocate(uint16_t index) {
        allocated.insert(index);
        allocCount++;
    }

    void DRAMFree(uint16_t index) {
        allocated.erase(index);
        freeCount++;
    }

    void updateDramUsage() {
        uint16_t size = allocated.size();

        if (dramUsage.find(size) == dramUsage.end()) {
            dramUsage[size] = 1;
        } else {
            dramUsage[size]++;
        }
    }


public:
    PhoenixController(gzFile output_trace, ofstream &output_log, int twsSize, bool useTrueLRU) : Controller(output_trace, output_log), tag_working_set(), allocated(), twsSize(twsSize), useTrueLRU(useTrueLRU), dramUsage(), allocCount(0), freeCount(0) {}

    void setup(Decoder &decoder) override {
        Cacheline superroot_line, root_line, leaf_line;
        uint8_t superroot_byte, root_byte;
        bool no_leaves_set;
        uint16_t index = 0;      // block index

        superroot_byte = 0;
        for (uint64_t rl = 0; rl < ROOT_TABLE_SIZE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                root_byte = 0;
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags
                    root_byte <<= 1;

                    // focus on this single root tag
                    decoder.getInitialTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    no_leaves_set = isClear(leaf_line);

                    memory.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) + ROOT_TABLE_SIZE + SUPERROOT_TABLE_SIZE, leaf_line);     // insert the leaf line into the cache

                    root_byte |= no_leaves_set ? 0 : 1;
                }

                root_line[rb] = root_byte;      // push the root byte into the line
            }

            memory.set(rl + SUPERROOT_TABLE_SIZE, root_line);     // insert the root line into the cache

            superroot_byte <<= 1;
            superroot_byte |= isClear(root_line) ? 0 : 1;
            if ((rl / TAG_CACHE_LINE_SIZE) % 8 == 7) {
                superroot_line[rl / TAG_CACHE_LINE_SIZE / 8] = superroot_byte;
                superroot_byte = 0;
            }

            if (!isClear(root_line)) allocated.insert(index);
            index++;
        }
        memory.set(0, superroot_line);

        updateDramUsage();
    };

    uint16_t handleRead(memAccess ax) override {
        uint64_t superroot_base_addr, superroot_cacheline_index, root_base_addr, root_cacheline_index, leaf_base_addr, leaf_cacheline_index;
        Cacheline superroot_line, root_line, leaf_line;
        pair<uint64_t, uint16_t> result;
        bool superroot_tag, root_tag;
        uint16_t tags;
        uint16_t index = blockIndex(ax.addr);

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                if (!isInTWS(index)) {
                    result = translateToSuperrootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    superroot_line = memory.doRead(superroot_base_addr);

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);
                    if (!superroot_tag) {
                        // tags must be 0
                        tags = 0;
                        break;
                    }
                }

                result = translateToRootAddr(ax.addr);
                root_base_addr = result.first;
                root_cacheline_index = result.second;
                root_line = memory.doRead(root_base_addr);

                root_tag = getTag(root_line, root_cacheline_index);
                if (!root_tag) {
                    // tags must be 0
                    tags = 0;
                    break;
                }

                result = translateToLeafAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;
                leaf_line = memory.doRead(leaf_base_addr);

                tags = getTags(leaf_line, leaf_cacheline_index);

                if (!tags) {
                    //cout << "read ";
                    updateTWS(index);
                }

                break;

            case ACCESS_TYPE_WRITE:
                assert(false);  // wrong call was made!
        }

        updateDramUsage();
        return tags;
    }

    void handleWrite(memAccess ax) override {
        uint64_t superroot_base_addr, superroot_cacheline_index, root_base_addr, root_cacheline_index, leaf_base_addr, leaf_cacheline_index;
        Cacheline superroot_line, root_line, leaf_line;
        pair<uint64_t, uint16_t> result;
        bool superroot_tag, root_tag;
        uint16_t index = blockIndex(ax.addr);

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                assert(false);  // wrong call was made!

            case ACCESS_TYPE_WRITE:
                if (ax.tags == 0) {
                    result = translateToSuperrootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    superroot_line = memory.doRead(superroot_base_addr);

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);
                    if (!superroot_tag) {
                        // tags already 0
                        break;
                    }

                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    root_line = memory.doRead(root_base_addr);

                    root_tag = getTag(root_line, root_cacheline_index);
                    if (!root_tag) {
                        // tags already 0
                        break;
                    }

                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;
                    leaf_line = memory.doRead(leaf_base_addr);

                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);

                    if (!isClear(leaf_line)) {
                        memory.doWrite(leaf_base_addr, leaf_line);
                        break;
                    }

                    /*
                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    root_line = cache.doRead(root_base_addr);
                     */

                    modifyTag(root_line, root_cacheline_index, 0);
                    memory.doWrite(root_base_addr, root_line);

                    if (!isClear(root_line)) break;

                    /*
                    result = translateToSuperRootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    superroot_line = cache.doRead(superroot_base_addr);
                     */

                    modifyTag(superroot_line, superroot_cacheline_index, 0);
                    memory.doWrite(superroot_base_addr, superroot_line);

                    if (!isInTWS(index)) {
                        DRAMFree(index);
                    }

                } else {
                    bool wasInTWS = isInTWS(index);
                    updateTWS(index);

                    result = translateToSuperrootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    superroot_line = memory.doRead(superroot_base_addr);

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);

                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;

                    if (!superroot_tag) {
                        if (!wasInTWS) {
                            DRAMAllocate(index);
                        }
                        modifyTag(superroot_line, superroot_cacheline_index, 1);
                        memory.doWrite(superroot_base_addr, superroot_line);

                        root_line = Cacheline{};    // a fully-zero cacheline

                    } else {
                        root_line = memory.doRead(root_base_addr);
                        root_tag = getTag(root_line, root_cacheline_index);
                    }

                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;

                    if (!superroot_tag || !root_tag) {
                        modifyTag(root_line, root_cacheline_index, 1);
                        memory.doWrite(root_base_addr, root_line);
                        leaf_line = Cacheline{};    // a fully-zero cacheline
                    } else {
                        leaf_line = memory.doRead(leaf_base_addr);
                    }

                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);
                    memory.doWrite(leaf_base_addr, leaf_line);
                    break;
                }
        }

        updateDramUsage();
    }

    void reportStats() override {
        uint64_t tot = 0, count = 0;
        uint16_t v;
        uint16_t max = 0;
        uint16_t min = 0xffff;   // MAX 16 BIT INT

        for (auto & it : dramUsage) {
            v = it.first;
            if (v > max) max = v;
            if (v < min) min = v;
            count += it.second;
            tot += v * it.second;
        }
        float avg = ((float) tot) / ((float) count);

        cout << "REPORT: === DRAM USAGE ===" << endl;
        cout << "REPORT: MAX BLOCKS:  " << max << endl;
        cout << "REPORT: MIN BLOCKS:  " << min << endl;
        cout << "REPORT: AVG BLOCKS:  " << avg << endl;
        cout << "REPORT: ALLOC COUNT: " << allocCount << endl;
        cout << "REPORT: FREE  COUNT: " << freeCount << endl;
    }
};