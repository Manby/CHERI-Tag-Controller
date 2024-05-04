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

constexpr uint64_t S_SUPERROOT_TABLE_SIZE = ROOT_TABLE_SIZE >> 6;

class PhoenixSController : public Controller {
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

        uint64_t leaf_base = (((addr_base / 128) >> 6) << 6) + S_SUPERROOT_TABLE_SIZE + ROOT_TABLE_SIZE;  // base address of the tag cacheline (byte address)
        uint16_t leaf_offset = ((addr_base>>6)*4) % 512;         // index into the tag cacheline (bit-index; multiple of 4 in [0,512))
        //cout << "LEAF ADDR " << leaf_base << endl;
        return {leaf_base, leaf_offset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

    static std::pair<uint64_t, uint16_t> translateToRootAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t root_base = (((addr_base / 65536) >> 6) << 6) + S_SUPERROOT_TABLE_SIZE;  // base address of the tag cacheline (byte address)
        uint16_t root_offset = ((addr_base>>6)*4 / 512) % 512;         // index into the tag cacheline (bit-index; in [0,512))
        //cout << "ROOT ADDR " << root_base << endl;
        return {root_base, root_offset};
    }

    static std::pair<uint64_t, uint16_t> translateToSuperrootAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t superroot_base = ((addr_base / 4194304) >> 6) << 6;  // base address of the tag cacheline (byte address)
        uint16_t superroot_offset = ((addr_base >> 6) * 4 / 512 / 64) % 512;         // index into the tag cacheline (bit-index; in [0,512))
        //cout << "SPRT ADDR " << superroot_base << endl;
        return {superroot_base, superroot_offset};
    }

    uint16_t blockIndex(uint64_t addr) {
        return (addr / 8192) >> 6;     // leaves only the top 12 bits of the addr (so its in the range [0,4096))
    }

    bool isBlockClear(uint16_t index) {
        //cout << "reading bi " << 64*(index/512) << endl;
        Cacheline line = memory.doRead(64*(index/512));
        //cout << "done" << endl;
        return !getTag(line, index%512);
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

        if (tag_working_set.size() == twsSize) {
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
    PhoenixSController(gzFile output_trace, ofstream &output_log, int twsSize, bool useTrueLRU) : Controller(output_trace, output_log), tag_working_set(), allocated(), twsSize(twsSize), useTrueLRU(useTrueLRU), dramUsage(), allocCount(0), freeCount(0) {}

    void setup(Decoder &decoder) override {
        Cacheline superroot_line, root_line, leaf_line;
        uint8_t superroot_byte = 0, root_byte = 0;
        uint16_t index = 0;      // block index
        int a=0, b=0, c=0, d=0, e=0;
        bool clear;

        for (uint64_t tag = 0; tag < TAG_TABLE_SIZE*8; tag += TAG_CACHE_LINE_SIZE*8) {
            decoder.getInitialTags(tag, TAG_CACHE_LINE_SIZE*8, leaf_line);


            memory.set(tag/8 + ROOT_TABLE_SIZE + S_SUPERROOT_TABLE_SIZE, leaf_line);     // insert the leaf line into the cache

            if (true) {                             // doing root bit
                root_byte <<= 1;
                root_byte |= isClear(leaf_line) ? 0 : 1;
            }
            if (((tag/512)+1) % 8 == 0) {                             // 8th doing root line
                root_line[a++] = root_byte;      // push the root byte into the line
                root_byte = 0;
            }
            if (((tag/512)+1) % (8*64) == 0) {                             // doing root line
                memory.set((b++)*64 + S_SUPERROOT_TABLE_SIZE, root_line);     // insert the leaf line into the cache
                a=0;
            }

            if (((tag/512)+1) % 64 == 0) {                             // doing superroot bit   (every 8 bytes of root tags)
                superroot_byte <<= 1;
                clear = eightBytesClear(root_line, c++);
                superroot_byte |= clear ? 0 : 1;
                if (!clear) allocated.insert(index);
            }
            if (((tag/512)+1) % (64*8) == 0) {                             // 8th doing superroot line
                superroot_line[d++] = superroot_byte;      // push the root byte into the line
                superroot_byte = 0;
                c=0;
            }
            if (((tag/512)+1) % (64*8*64) == 0) {                             // doing superroot line
                memory.set((e++)*64, superroot_line);     // insert the leaf line into the cache
                d=0;
            }
        }

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
                    //cout << "reading sr " << superroot_base_addr << endl;
                    superroot_line = memory.doRead(superroot_base_addr);
                    //cout << "done" << endl;

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);
                    if (!superroot_tag) {
                        // tags must be 0
                        tags = 0;
                        break;
                    }

                    updateTWS(index);
                }

                result = translateToRootAddr(ax.addr);
                root_base_addr = result.first;
                root_cacheline_index = result.second;
                //cout << "reading r " << root_base_addr << endl;
                root_line = memory.doRead(root_base_addr);
                //cout << "done" << endl;

                root_tag = getTag(root_line, root_cacheline_index);
                if (!root_tag) {
                    // tags must be 0
                    tags = 0;
                    break;
                }

                result = translateToLeafAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;
                //cout << "reading l " << leaf_base_addr << endl;
                leaf_line = memory.doRead(leaf_base_addr);
                //cout << "done" << endl;

                tags = getTags(leaf_line, leaf_cacheline_index);
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
                    //cout << "reading sr " << superroot_base_addr << endl;
                    superroot_line = memory.doRead(superroot_base_addr);
                    //cout << "done" << endl;

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);
                    if (!superroot_tag) {
                        // tags already 0
                        break;
                    }

                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    //cout << "reading r " << root_base_addr << endl;
                    root_line = memory.doRead(root_base_addr);
                    //cout << "done" << endl;

                    root_tag = getTag(root_line, root_cacheline_index);
                    if (!root_tag) {
                        // tags already 0
                        break;
                    }

                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;
                    //cout << "reading l " << leaf_base_addr << endl;
                    leaf_line = memory.doRead(leaf_base_addr);
                    //cout << "done" << endl;

                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);

                    if (!isClear(leaf_line)) {
                        //cout << "writing l " << leaf_base_addr << endl;
                        memory.doWrite(leaf_base_addr, leaf_line);
                        //cout << "done" << endl;
                        break;
                    }

                    /*
                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;
                    root_line = cache.doRead(root_base_addr);
                     */

                    modifyTag(root_line, root_cacheline_index, 0);
                    //cout << "writing r " << root_base_addr << endl;
                    memory.doWrite(root_base_addr, root_line);
                    //cout << "done" << endl;

                    if (!eightBytesClear(root_line, superroot_cacheline_index%8)) break;

                    /*
                    result = translateToSuperRootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    superroot_line = cache.doRead(superroot_base_addr);
                     */

                    modifyTag(superroot_line, superroot_cacheline_index, 0);
                    //cout << "writing sr " << superroot_base_addr << endl;
                    memory.doWrite(superroot_base_addr, superroot_line);
                    //cout << "done" << endl;

                    if (!isInTWS(index)) {
                        DRAMFree(index);
                    }

                } else {
                    result = translateToSuperrootAddr(ax.addr);
                    superroot_base_addr = result.first;
                    superroot_cacheline_index = result.second;
                    //cout << "reading sr " << superroot_base_addr << endl;
                    superroot_line = memory.doRead(superroot_base_addr);
                    //cout << "done" << endl;

                    superroot_tag = getTag(superroot_line, superroot_cacheline_index);

                    result = translateToRootAddr(ax.addr);
                    root_base_addr = result.first;
                    root_cacheline_index = result.second;

                    if (!superroot_tag) {
                        if (!isInTWS(index)) {
                            updateTWS(index);
                            DRAMAllocate(index);
                        }
                        modifyTag(superroot_line, superroot_cacheline_index, 1);
                        //cout << "writing sr " << superroot_base_addr << endl;
                        memory.doWrite(superroot_base_addr, superroot_line);
                        //cout << "done" << endl;

                        root_line = Cacheline{};    // a fully-zero cacheline

                    } else {
                        //cout << "reading r " << root_base_addr << endl;
                        root_line = memory.doRead(root_base_addr);
                        //cout << "done" << endl;
                        root_tag = getTag(root_line, root_cacheline_index);
                    }

                    result = translateToLeafAddr(ax.addr);
                    leaf_base_addr = result.first;
                    leaf_cacheline_index = result.second;

                    if (!superroot_tag || !root_tag) {
                        modifyTag(root_line, root_cacheline_index, 1);
                        //cout << "writing r " << root_base_addr << endl;
                        memory.doWrite(root_base_addr, root_line);
                        //cout << "done" << endl;
                        leaf_line = Cacheline{};    // a fully-zero cacheline
                    } else {
                        //cout << "reading l " << leaf_base_addr << endl;
                        leaf_line = memory.doRead(leaf_base_addr);
                        //cout << "done" << endl;
                    }

                    modifyTags(leaf_line, leaf_cacheline_index, ax.tags);
                    //cout << "writing l " << leaf_base_addr << endl;
                    memory.doWrite(leaf_base_addr, leaf_line);
                    //cout << "done" << endl;
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