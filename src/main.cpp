#include "log/log.hpp"

#include <iostream>

int main() {
    logger::default_setup();

    LOG_INFO("Hello Computer Graphics!");
    return 0;
}
