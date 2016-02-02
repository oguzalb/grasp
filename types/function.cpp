#include "function.h"
extern Class *func_type;

Function::Function(std::vector<std::string> &codes, int startp) {
    this->type = func_type;
    this->codep = startp;
    this->codes = codes;
}

