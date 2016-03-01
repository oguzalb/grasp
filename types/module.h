#ifndef GR_MODULE
#define GR_MODULE
#include "object.h"
#include "../vm.h"

class Module : public Object {
    public:
    std::vector<std::string> *codes;
    Module(std::vector<std::string> *codes);
};

void init_module();

#endif
