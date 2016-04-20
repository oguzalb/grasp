#ifndef GR_MODULE
#define GR_MODULE
#include "object.h"
#include "../vm.h"

class Module : public Object {
    public:
    std::vector<unsigned char> *codes;
    std::vector<Object *> co_consts;
    Module(std::vector<unsigned char> *codes);
};

void init_module();

#endif
