#include "builtin_function.h"
#include "class.h"
extern Class *builtinfunc_type;


BuiltinFunction::BuiltinFunction(void(*function)(), int param_count) {
    this->type = builtinfunc_type;
    this->function = function;
    this->param_count = param_count;
}

void init_builtin_func() {
    builtinfunc_type = new Class("builtin_func", NULL, 0);
}
