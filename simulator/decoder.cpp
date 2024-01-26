#include <cassert>
#include "decoder.h"
#include "controller.h"

#define QEMU_BASE_ADDRESS 0x80000000 // 2^31

Decoder::Decoder(FILE *initial_accesses, FILE *trace) : initial_accesses(initial_accesses), trace(trace) {} // TODO: should this be a std::move instead of a construction?

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

size_t Decoder::read_llc_misses(access *buffer, size_t n) {
    cout << "in func" << endl;
    uint8_t intermediate[16*n];  // llcMiss is 16 bytes
    // TODO: could the reading from the file and the struct conversion happen in parallel? i.e. multithreading
    cout << "attempting the fread" << endl;
    size_t numRead = fread(intermediate, sizeof(llcMiss), n, trace);

    for (int i = 0; i < numRead*16; i += 16) {
        auto type = (llcMissType) intermediate[i];
        uint16_t size = intermediate[i+2] + ((uint16_t) intermediate[i+3] << 8);
        uint16_t tags = intermediate[i+4] + ((uint16_t) intermediate[i+5] << 8);
        uint16_t tags_known = intermediate[i+6] + ((uint16_t) intermediate[i+7] << 8);
        uint64_t addr = 0;
        for (int j = 0; j < 8; ++j) {
            addr += ((uint64_t) intermediate[i+8+j]) << (8*j);
        }
        addr -= QEMU_BASE_ADDRESS;

        llcMiss miss = llcMiss(type, size, tags, tags_known, addr);
        buffer[i/16] = furnish(miss);
    }

    return numRead;
}

bool Decoder::getInitialTag(uint64_t index) {
    assert(index < 134217728); // 2^27 bytes

    // TODO: 2 proper casts
    fseek(initial_accesses, index, SEEK_SET);

    uint8_t intermediate;
    cout << "reading out initial access data at index " << index << endl;
    size_t numRead = fread(&intermediate, 1, 1, initial_accesses);

    assert(numRead == 1);

    bool tag = intermediate >> 7;
    int8_t type = intermediate & 0x7f;

    switch (type) {
        case INITIAL_ACCESS_TYPE_INSTR:
        case INITIAL_ACCESS_TYPE_LOAD:
        case INITIAL_ACCESS_TYPE_STORE:
            return false;
        case INITIAL_ACCESS_TYPE_CLOAD:
        case INITIAL_ACCESS_TYPE_CSTORE:
            return tag;
        default:
            cout << "type was " << (int) type << endl;
            return false; // initial access file claims this address was never accessed (type is -1), so we can return any value
    }
}

access Decoder::furnish(llcMiss miss) {
    // TODO: refactor assert (?; and change == 15 check if necessary)
    assert(miss.size == 64);
    assert(miss.addr % 64 == 0);    // should be a 64-byte aligned address, as each cacheline is 64 bytes long

    if (miss.tags_known != 15) {
        int i = 0;
        cout << "this miss has unknown tags: " << miss.tags_known << endl;
        for (uint16_t base = 16; base >= 1; base >>= 1) {   // left-to-right; MSB-to-LSB
            if (!(miss.tags_known & base)) {
                cout << "need to look up tag " << i << " for cacheline base address " << miss.addr << endl;
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

    return access(
            miss.type == LLC_MISS_TYPE_READ ? ACCESS_TYPE_READ : ACCESS_TYPE_WRITE,
            miss.size,
            miss.tags,
            miss.addr
            );
}

size_t Simulator::processTrace(Decoder &decoder, Controller &controller, access *buffer, size_t n) {
    size_t i = 0;
    while (i < n) {
        controller.handleMemoryAccess(buffer[i]);  // calls are automatically inlined
        ++i;
    }

    return i;
}
