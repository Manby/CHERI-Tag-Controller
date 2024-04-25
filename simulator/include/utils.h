#pragma once

#include <bitset>
#include "schema.h"

bool isClear(Cacheline &line);
void modifyTags(Cacheline &line, uint16_t bit_index, uint16_t tags);
void modifyTag(Cacheline &line, uint16_t bit_index, bool tag);
bool getTag(Cacheline &line, uint16_t bit_index);

// computes whether the cacheline contains any set bits or not
bool isClear(Cacheline &line) {
    bool clear = true;
    for (uint8_t byte: line) {
        if (byte != 0) {
            clear = false;
            break;
        }
    }
    return clear;
}

void modifyTags(Cacheline &line, uint16_t bit_index, uint16_t tags) {   // modify 4 tags at a time
    std::bitset<8> byte = line[bit_index / 8];
    byte[7 - (bit_index % 8)] = (tags & 8) ? 1 : 0;
    byte[7 - ((bit_index + 1) % 8)] = (tags & 4) ? 1 : 0;
    byte[7 - ((bit_index + 2) % 8)] = (tags & 2) ? 1 : 0;
    byte[7 - ((bit_index + 3) % 8)] = (tags & 1) ? 1 : 0;
    line[bit_index / 8] = (uint8_t) byte.to_ulong();
}

void modifyTag(Cacheline &line, uint16_t bit_index, bool tag) {
    std::bitset<8> byte = line[bit_index / 8];
    byte[7 - (bit_index % 8)] = tag ? 1 : 0;
    line[bit_index / 8] = (uint8_t) byte.to_ulong();
}

bool getTag(Cacheline &line, uint16_t bit_index) {
    std::bitset<8> byte = line[bit_index / 8];
    return byte[7 - (bit_index % 8)];
}
