#include "utils/plot.h"
#include "log/log.h"
#include <iostream>

int main() {
    int a = plotCircle(5);
    std::printf("H a = %d\n", a);
    std::string msg("Hello world.");
    LOG_D("main", msg);

    return 0;
}