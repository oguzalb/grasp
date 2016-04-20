#include "module.h"
extern Class *module_type;
extern Object *none;

Module::Module(std::vector<unsigned char> *codes) {
    this->type = module_type;
    this->codes = codes;
    this->co_consts.push_back(none);
}

void init_module() {
    module_type = new Class("module", object_new, 1);
}
