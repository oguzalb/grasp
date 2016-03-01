#include "module.h"
extern Class *module_type;

Module::Module(std::vector<std::string> *codes) {
    this->type = module_type;
    this->codes = codes;
}

void init_module() {
    module_type = new Class("module", NULL);
}
