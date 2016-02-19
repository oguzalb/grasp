#ifndef GR_FUNCTION
#define GR_FUNCTION
#include <vector>
#include "object.h"

class Function : public Object {
    public:
    int codep;
    std::vector<std::string> codes;
    string name;
    Function(std::vector<std::string> &codes, int startp, string name);
};

void init_function();

#endif
