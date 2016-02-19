#include "function.h"
#include "../vm.h"

extern Class *func_type;

Function::Function(std::vector<std::string> &codes, int startp, string name) {
    this->type = func_type;
    this->codep = startp;
    this->codes = codes;
    this->name = name;
}

void init_function() {
    func_type = new Class("func", NULL);
}
