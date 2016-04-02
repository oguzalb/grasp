#ifndef GR_BUILTIN_FUNC
#define GR_BUILTIN_FUNC
#include "object.h"

class BuiltinFunction : public Object {
    public:
    void (*function) ();
    BuiltinFunction(void(*function)(), int);
    int param_count;
};
void init_builtin_func();

#endif
