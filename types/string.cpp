#include "string.h"
#include "../vm.h"

extern Class *str_type;

String::String(string sval) {
    this->type = str_type;
    this->sval = sval;
}

void init_string() {
    str_type = new Class("str", NULL);
}
