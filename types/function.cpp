#include "function.h"
#include "../vm.h"
#include "module.h"

extern Class *func_type;

Function::Function(std::unordered_map<string, Object *> *globals, std::vector<std::string> &codes, int startp, string name, int locals_count, int param_count) {
    this->type = func_type;
    this->globals = globals;
    this->codep = startp;
    this->codes = codes;
    this->name = name;
    this->locals_count = locals_count;
    this->param_count = param_count;
}

void init_function() {
    func_type = new Class("func", NULL, 0);
}
