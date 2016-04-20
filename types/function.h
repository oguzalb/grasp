#ifndef GR_FUNCTION
#define GR_FUNCTION
#include <vector>
#include "object.h"

class Module;

class Function : public Object {
    public:
    int codep;
    string name;
    int locals_count;
    Function(std::unordered_map<string, Object *> *globals, Module *module, int startp, string name, int locals_count, int param_count);
    int param_count;
    std::unordered_map<string, Object *> *globals;
    Module *module;
};

void init_function();

#endif
