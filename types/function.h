#ifndef GR_FUNCTION
#define GR_FUNCTION
#include <vector>
#include "object.h"

class Function : public Object {
    public:
    int codep;
    std::vector<std::string> codes;
    string name;
    int locals_count;
    Function(std::vector<std::string> &codes, int startp, string name, int locals_count);
};

void init_function();

#endif
