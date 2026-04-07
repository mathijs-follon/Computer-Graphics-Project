#include "log/log.hpp"

int main() {
    logger::default_setup();

    LOG_INFO("Hello Computer Graphics!");
    return 0;
}
