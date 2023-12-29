#include "decoder.h"
#include "controller.h"

size_t read_init_accesses(FILE *file, initial_access *buffer, size_t n) {
    uint8_t intermediate[n];
    // TODO: could the reading from the file and the struct conversion happen in parallel? i.e. multithreading
    size_t numRead = fread(intermediate, 1, n, file);

    for (int i = 0; i < numRead; ++i) {
        bool tag = intermediate[i] >> 7;
        int8_t type = intermediate[i] & 0x7f;
        buffer[i] = initial_access(type, tag);
    }

    return numRead;
}

size_t read_llc_misses(FILE *file, llc_miss *buffer, size_t n) {
    uint8_t intermediate[16*n];  // llc_miss is 16 bytes
    // TODO: could the reading from the file and the struct conversion happen in parallel? i.e. multithreading
    size_t numRead = fread(intermediate, sizeof(llc_miss), n, file);

    for (int i = 0; i < numRead*16; i += 16) {
        auto type = (llc_miss_type) intermediate[i];
        uint16_t size = intermediate[i+2] + ((uint16_t) intermediate[i+3] << 8);
        uint16_t tags = intermediate[i+4] + ((uint16_t) intermediate[i+5] << 8);
        uint16_t tags_known = intermediate[i+6] + ((uint16_t) intermediate[i+7] << 8);
        uint64_t addr = 0;
        for (int j = 0; j < 8; ++j) {
            addr += ((uint64_t) intermediate[i+8+j]) << (8*j);
        }
        buffer[i/16] = llc_miss(type, size, tags, tags_known, addr);
    }

    return numRead;
}

access convert(llc_miss miss) {
    return access{};
}

size_t Simulator::processTrace(Controller &controller, llc_miss *buffer, size_t n) {
    size_t i = 0;
    while (i < n) {
        access ax = convert(buffer[i]);
        controller.handleMemoryAccess(ax);  // calls are automatically inlined
        ++i;
    }

    return i;
}
