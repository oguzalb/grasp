#ifndef GR_BOOL
#define GR_BOOL
#include "object.h"
#include "../vm.h"

class Bool : public Object {
    public:
    int bval;
    Bool(int bval);
};

void init_bool();

#endif
