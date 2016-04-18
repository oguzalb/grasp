#ifndef GR_FUNCTION
#define GR_FUNCTION
#include <vector>
#include "object.h"

class Module;

class Function : public Object {
    public:
    int codep;
    std::vector<unsigned char> codes;
    string name;
    int locals_count;
    Function(std::unordered_map<string, Object *> *globals, std::vector<Object *> *co_consts, std::vector<unsigned char> &codes, int startp, string name, int locals_count, int param_count);
    int param_count;
    std::unordered_map<string, Object *> *globals;
    std::vector<Object *> *co_consts;
};

void init_function();

#endif
