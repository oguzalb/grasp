#include "string.h"
#include "../vm.h"

extern Class *str_type;

String::String(string sval) {
    this->type = str_type;
    this->sval = sval;
}

void str_add() {
    String *other = POP_TYPE(String, str_type);
    String *self = POP_TYPE(String, str_type);
    // TODO exc
    PUSH(new String(self->sval + other->sval));
}

void init_string() {
    str_type = new Class("str", NULL);
    str_type->setmethod("__add__", str_add);
}
