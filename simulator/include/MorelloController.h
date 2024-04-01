/*
 * This class implements the controller used by Arm's Morello Processors
 */

#pragma once

#include <bitset>
#include <iostream>
#include <iomanip>
#include "controller.h"
#include "decoder.h"
#include "schema.h"

class MorelloController : public Controller {
private:
    bool cache_type;
public:
    MorelloController(Decoder &decoder, ofstream &output_trace, ofstream &output_log, bool cache_type) : Controller(output_trace, output_log), cache_type(cache_type) {
        array<uint8_t, TAG_CACHE_LINE_SIZE> root_line, leaf_line;

        for (uint64_t rl = 0; rl < LEAF_TABLE_BASE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags

                    // focus on this single root tag
                    decoder.getTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    //cout << "SETTING LEAF @ " << TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt) << endl;
                    cache.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt), leaf_line);     // insert the leaf line into the cache
                }
            }
        }
    };

    void handleMemoryAccess(access ax) override {
        uint64_t leaf_index, leaf_base_addr, leaf_cacheline_index;
        array<uint8_t, TAG_CACHE_LINE_SIZE> root_line, leaf_line;
        bitset<8> leaf_byte;
        uint8_t leaf_tags;
        //cout << "AX " << setfill('0') << setw(2) << ax.tags << " " << setw(16) << ax.addr << " " << setw(1) << (cache_type ? 1 : 0) << endl;
        switch (ax.type) {
            case ACCESS_TYPE_READ:
                DBG cout << "\nREAD @ " << ax.addr << endl;

                leaf_index = ax.addr >> 4;
                leaf_base_addr = ((leaf_index - (leaf_index % (8 * TAG_CACHE_LINE_SIZE)))
                        >> 3);   // base address of the cacheline
                leaf_line = cache.peek(leaf_base_addr);

                leaf_cacheline_index = leaf_index % (8*TAG_CACHE_LINE_SIZE);    // index of the first of the 4 tags /within the cacheline/
                leaf_byte = leaf_line[leaf_cacheline_index / 8];
                //assert(leaf_cacheline_index % 4 == 0);    is true
                leaf_tags = 8 * leaf_byte[leaf_cacheline_index % 8] +
                            4 * leaf_byte[(leaf_cacheline_index + 1) % 8] +
                            2 * leaf_byte[(leaf_cacheline_index + 2) % 8] +
                            1 * leaf_byte[(leaf_cacheline_index + 3) % 8];

                //cout << "READ ";
                if (((leaf_tags == 0) && !cache_type) ||
                        ((leaf_tags != 0) && cache_type)) {// if we are responsible for handling this one, then log it
                    //cout << "ME" << endl;
                    cache.logRead(leaf_base_addr);
                } else {
                    //cout << "NO" << endl;
                }

                return; // we would return whatever the leaf is

            case ACCESS_TYPE_WRITE:
                DBG cout << "\nWRITE @ " << ax.addr << endl;

                leaf_index = ax.addr >> 4;
                leaf_base_addr = ((leaf_index - (leaf_index % (8 * TAG_CACHE_LINE_SIZE)))
                        >> 3);   // base address of the cacheline
                leaf_line = cache.peek(leaf_base_addr);

                leaf_cacheline_index = leaf_index % (8*TAG_CACHE_LINE_SIZE);    // index of the first of the 4 tags /within the cacheline/
                leaf_byte = leaf_line[leaf_cacheline_index / 8];
                //assert(leaf_cacheline_index % 4 == 0);    is true
                leaf_tags = 8 * leaf_byte[leaf_cacheline_index % 8] +
                            4 * leaf_byte[(leaf_cacheline_index + 1) % 8] +
                            2 * leaf_byte[(leaf_cacheline_index + 2) % 8] +
                            1 * leaf_byte[(leaf_cacheline_index + 3) % 8];

                //cout << "WRIT ";
                leaf_byte[leaf_cacheline_index % 8] = (ax.tags & 8) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 1) % 8] = (ax.tags & 4) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 2) % 8] = (ax.tags & 2) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 3) % 8] = (ax.tags & 1) ? 1 : 0;
                // TODO: check above not backwards
                leaf_line[leaf_cacheline_index / 8] = (uint8_t) leaf_byte.to_ulong();
                cache.set(leaf_base_addr, leaf_line);

                if (((leaf_tags == 0) && !cache_type) ||
                    ((leaf_tags != 0) && cache_type)) {// if we are responsible for handling this one, then log it
                    //cout << "ME" << endl;
                    cache.logWrite(leaf_base_addr);
                } else {
                    //cout << "NO" << endl;
                }
                return;

            //default:
                //if (ax.type != ACCESS_TYPE_READ && ax.type != ACCESS_TYPE_WRITE) cout << "###";
                //cout << "!!!" << endl;
        }
    }
};