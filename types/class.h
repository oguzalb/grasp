#ifndef GR_CLASS
#define GR_CLASS
#include "builtin_function.h"

class Class : public BuiltinFunction {
    public:
    Class(string type_name);
    string type_name;
};

#endif
