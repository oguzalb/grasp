#include "string.h"
#include "../vm.h"

extern Class *str_type;
extern Object *trueobject;
extern Object *falseobject;

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

void str_equals() {
    String *o1 = POP_TYPE(String, str_type);
    String *o2 = POP_TYPE(String, str_type);
    if (o1->sval == o2->sval) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void str_hash() {
    String *self = POP_TYPE(String, str_type);
    // TODO exc
    PUSH(new Int(std::hash<string>()(self->sval)));
}

void str_contains() {
    String *other = POP_TYPE(String, str_type);
    String *self = POP_TYPE(String, str_type);
    if (self->sval.find(other->sval) != std::string::npos) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void init_string() {
    str_type = new Class("str", NULL);
    str_type->setmethod("__add__", str_add);
    str_type->setmethod("__equals__", str_equals);
    str_type->setmethod("__hash__", str_hash);
    str_type->setmethod("__contains__", str_contains);
}
