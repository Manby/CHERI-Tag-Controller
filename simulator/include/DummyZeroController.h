/*
 * This class implements a controller that always reads the tags as zeroes.
 */

#pragma once

#include <iostream>
#include "FlatTableController.h"
#include "schema.h"
#include "utils.h"

class DummyZeroController : public FlatTableController {
public:
    DummyZeroController(gzFile output_trace, ofstream &output_log) : FlatTableController(output_trace, output_log) {}

    uint16_t handleRead(memAccess ax) override {
        switch (ax.type) {
            case ACCESS_TYPE_READ:
                return 0b0000;

            case ACCESS_TYPE_WRITE:
                assert(false);  // wrong call was made!
        }

        assert(false);
        return 0xffff;  // should never get here!
    }

    void handleWrite(memAccess ax) override {}
};