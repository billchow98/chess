#include "common.h"
#include "hash.h"
#include "lookup.h"
#include "search.h"
#include "uci.h"

using namespace tuna;

void init() {
    hash::init();
    lookup::init();
    search::init();
}

i32 main() {
    init();
    return uci::run_loop();
}
