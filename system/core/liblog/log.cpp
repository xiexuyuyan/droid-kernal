#include "log/log.h"
void LOG_D(const std::string& TAG, const std::string& text) {
    std::cout<<TAG<<": "<<text<<std::endl;
}