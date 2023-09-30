#include "state.h"

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *buf, size_t len) {
    state state = {};
    state_create(&state);

    terminal_parse(&state.terminal, (char *) buf, len);

    state_destroy(&state);

    return 0;
}
