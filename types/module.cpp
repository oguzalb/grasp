#include "module.h"
extern Class *module_type;

Module::Module(std::vector<Object *> *co_consts, std::vector<unsigned char> *codes) {
    this->type = module_type;
    this->codes = codes;
    this->co_consts = co_consts;
}

void init_module() {
    module_type = new Class("module", object_new, 1);
}
