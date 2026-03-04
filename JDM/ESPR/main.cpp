//
// Created by david on 2026-02-28.
//

#include <iostream>
#include "audio.h"


int main() {
    UDPHandler handler = UDPHandler();
    handler.setUpStream(false, false, false);
}