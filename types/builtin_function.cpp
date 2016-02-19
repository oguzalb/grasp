#include "builtin_function.h"
#include "class.h"
extern Class *builtinfunc_type;


BuiltinFunction::BuiltinFunction(void(*function)()) {
    this->type = builtinfunc_type;
    this->function = function;
}

void init_builtin_func() {
    builtinfunc_type = new Class("builtin_func", NULL);
}
