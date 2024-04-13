/*
 * This class implements the controller described in the Efficient Tagged Memory Paper.
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8119285&tag=1
 */

#pragma once

#include <bitset>
#include <iostream>
#include "controller.h"
#include "decoder.h"
#include "schema.h"

class ETMController : public Controller {
public:
    ETMController(Decoder &decoder, ofstream &output_trace, ofstream &output_log) : Controller(output_trace, output_log) {
        array<uint8_t, TAG_CACHE_LINE_SIZE> root_line, leaf_line;
        uint8_t root_byte;
        bool no_leaves_set;

        for (uint64_t rl = 0; rl < LEAF_TABLE_BASE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                root_byte = 0;
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags
                    root_byte <<= 1;

                    // focus on this single root tag
                    decoder.getTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    // process the cacheline of leaf tags
                    no_leaves_set = true;
                    for (int lb = 0; lb < TAG_CACHE_LINE_SIZE; ++lb) {
                        if (leaf_line[lb] != 0) {
                            no_leaves_set = false;
                            break;
                        }
                    }

                    //cout << "SETTING LEAF @ " << TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) << " + LEAF_TABLE_BASE" << endl;
                    cache.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) + LEAF_TABLE_BASE, leaf_line);     // insert the leaf line into the cache

                    root_byte |= no_leaves_set ? 0 : 1;
                    //cout << no_leaves_set;
                }

                root_line[rb] = root_byte;      // push the root byte into the line
            }

            //cout << "SETTING ROOT @ " << rl << " (of " << (uint64_t) LEAF_TABLE_BASE << ", not inclusive)" << endl;
            cache.set(rl, root_line);     // insert the root line into the cache
        }
    };

    void handleMemoryAccess(memAccess ax) override {
        uint64_t root_index, root_base_addr, root_cacheline_index, leaf_index, leaf_base_addr, leaf_cacheline_index;
        array<uint8_t, TAG_CACHE_LINE_SIZE> root_line, leaf_line;
        bitset<8> root_byte, leaf_byte;
        bool root_tag, leaf_tag;
        assert(ax.addr % 64 == 0);
        switch (ax.type) {
            case ACCESS_TYPE_READ:
                DBG cout << "\nREAD @ " << ax.addr << endl;
                //cache.logRead(translateAddrDataToTag(ax.addr, 0).first, ax.tags);
                root_index = (ax.addr >> 4) / (8*TAG_CACHE_LINE_SIZE); // index of the root tag we want;
                                                                       // 2^4 bytes per leaf tag * 8*TAG_CACHE_LINE_SIZE leaf tags per root tag
                root_base_addr = (root_index - (root_index % (8*TAG_CACHE_LINE_SIZE))) >> 3;   // base address of the cacheline
                root_line = cache.doRead(root_base_addr);

                root_cacheline_index = root_index % (8*TAG_CACHE_LINE_SIZE);
                root_byte = root_line[root_cacheline_index / 8];
                root_tag = root_byte[root_cacheline_index % 8];
                // TODO: Check that the above line isn't doing it backwards!

                if (!root_tag) {
                    DBG cout << "Root tag was zero; short circuit!" << endl;
                    return;   // we would return a 0 to the client
                }
                TST cout << "READING A NON-ZERO LINE at " << ax.addr << endl;

                leaf_index = ax.addr >> 4;        // just the index of the first tag of the 4 in the cacheline; an index into the entire leaf table
                                                    // will be a multiple of 4
                leaf_base_addr = ((leaf_index - (leaf_index % (8*TAG_CACHE_LINE_SIZE))) >> 3) + LEAF_TABLE_BASE;   // base address of the cacheline
                leaf_line = cache.doRead(leaf_base_addr);

                /*
                Below is what we /would/ do, but we don't need to

                leaf_cacheline_index = leaf_index % (8*TAG_CACHE_LINE_SIZE);    // index of the first of the 4 tags /within the cacheline/
                cout << "INDEX: " << leaf_cacheline_index << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                assert(leaf_cacheline_index % 4 == 0);

                leaf_byte = leaf_line[leaf_cacheline_index / 8];
                leaf_tag = leaf_byte[leaf_cacheline_index % 8];
                */

                return; // we would return whatever the leaf is

            case ACCESS_TYPE_WRITE:
                DBG cout << "\nWRITE @ " << ax.addr << endl;
                //cache.logWrite(translateAddrDataToTag(ax.addr, 0).first, ax.tags);
                root_index = (ax.addr >> 4) / (8*TAG_CACHE_LINE_SIZE); // index of the root tag we want;
                // 2^4 bytes per leaf tag * 8*TAG_CACHE_LINE_SIZE leaf tags per root tag
                root_base_addr = (root_index - (root_index % (8*TAG_CACHE_LINE_SIZE))) >> 3;   // base address of the cacheline
                root_line = cache.doRead(root_base_addr);

                root_cacheline_index = root_index % (8*TAG_CACHE_LINE_SIZE);
                root_byte = root_line[root_cacheline_index / 8];
                root_tag = root_byte[root_cacheline_index % 8];

                if (!root_tag && ax.tags == 0) {
                    DBG cout << "Root tag was zero, and we're writing zeroes; short circuit!" << endl;
                    return;  // we don't need to touch anything
                }

                if (ax.tags == 0) {
                    DBG cout << "We're writing zeroes, but root tag is 1; shall we clear the root?" << endl;
                    leaf_index = ax.addr >> 4;
                    leaf_base_addr = ((leaf_index - (leaf_index % (8*TAG_CACHE_LINE_SIZE))) >> 3) + LEAF_TABLE_BASE;   // base address of the cacheline
                    leaf_line = cache.doRead(leaf_base_addr);

                    leaf_cacheline_index = leaf_index % (8*TAG_CACHE_LINE_SIZE);
                    leaf_byte = leaf_line[leaf_cacheline_index / 8];

                    leaf_byte[leaf_cacheline_index % 8] = 0;
                    leaf_byte[(leaf_cacheline_index + 1) % 8] = 0;
                    leaf_byte[(leaf_cacheline_index + 2) % 8] = 0;
                    leaf_byte[(leaf_cacheline_index + 3) % 8] = 0;
                    // TODO: check above not backwards
                    leaf_line[leaf_cacheline_index / 8] = (uint8_t) leaf_byte.to_ulong();
                    cache.doWrite(leaf_base_addr, leaf_line);

                    for (int i = 0; i < TAG_CACHE_LINE_SIZE; ++i) {
                        if (leaf_line[i] != 0) {
                            DBG cout << "Nope -- found a 1" << endl;
                            return;  // cannot clear the root as there is a tag still set
                        }
                    }

                    DBG cout << "Found no 1s, so yes!" << endl;
                    // no leaf tags are set -- clear the root
                    root_byte[root_cacheline_index % 8] = 0;    // write a 0 in
                    root_line[root_cacheline_index / 8] = (uint8_t) root_byte.to_ulong();
                    // TODO: check above not backwards
                    cache.doWrite(root_base_addr, root_line);
                    return;
                }

                if (!root_tag) {
                    DBG cout << "Setting the root (it was cleared)" << endl;
                    root_byte[root_cacheline_index % 8] = 1;    // write a 1 in
                    root_line[root_cacheline_index / 8] = (uint8_t) root_byte.to_ulong();
                    // TODO: check above not backwards
                    cache.doWrite(root_base_addr, root_line);
                }

                leaf_index = (ax.addr >> 7) / (TAG_CACHE_LINE_SIZE);
                leaf_base_addr = ((leaf_index - (leaf_index % (8*TAG_CACHE_LINE_SIZE))) >> 3) + LEAF_TABLE_BASE;   // base address of the cacheline
                leaf_line = cache.doRead(leaf_base_addr);

                leaf_cacheline_index = leaf_index % (8*TAG_CACHE_LINE_SIZE);
                leaf_byte = leaf_line[leaf_cacheline_index / 8];
                leaf_tag = leaf_byte[leaf_cacheline_index % 8];
                if (!leaf_tag) {
                    DBG cout << "Setting the leaf (it was cleared)" << endl;
                    leaf_byte[leaf_cacheline_index % 8] = (ax.tags & 8) ? 1 : 0;
                    leaf_byte[(leaf_cacheline_index + 1) % 8] = (ax.tags & 4) ? 1 : 0;
                    leaf_byte[(leaf_cacheline_index + 2) % 8] = (ax.tags & 2) ? 1 : 0;
                    leaf_byte[(leaf_cacheline_index + 3) % 8] = (ax.tags & 1) ? 1 : 0;
                    // TODO: check above not backwards
                    leaf_line[leaf_cacheline_index / 8] = (uint8_t) leaf_byte.to_ulong();
                    cache.doWrite(leaf_base_addr, leaf_line);
                }
        }
    }
};