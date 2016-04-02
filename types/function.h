#ifndef GR_FUNCTION
#define GR_FUNCTION
#include <vector>
#include "object.h"

class Module;

class Function : public Object {
    public:
    int codep;
    std::vector<std::string> codes;
    string name;
    int locals_count;
    Function(std::unordered_map<string, Object *> *globals, std::vector<std::string> &codes, int startp, string name, int locals_count, int param_count);
    int param_count;
    std::unordered_map<string, Object *> *globals;
};

void init_function();

#endif
