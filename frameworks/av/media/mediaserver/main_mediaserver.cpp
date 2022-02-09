//
// Created by 23517 on 2022/1/26.
//
#include "utils/plot.h"
#include "log/log.h"
#include <iostream>

int main() {
    int a = plotCircle(5);
    std::printf("H a = %d\n", a);
    std::string msg("Hello world.");
    LOG_D("TAG", msg);
    return 0;
}