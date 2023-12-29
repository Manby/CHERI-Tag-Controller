//
// Created by kofi on 29/12/23.
//

#pragma once

#include "controller.h"

class DemoController : public Controller {
public:
    virtual void handleMemoryAccess(access ax) {
        cache.read();
        cache.write();
    }
};