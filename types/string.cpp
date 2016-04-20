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
    String *other = (String *)POP_TYPE(str_type);
    if (other == NULL)
        {POP();return;}
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        return;
    // TODO exc
    PUSH(new String(self->sval + other->sval));
}

void str_equals() {
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        {POP();return;}
    String *other = (String *)POP_TYPE(str_type);
    if (other == NULL)
        return;
    if (self->sval == other->sval) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void str_hash() {
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        return;
    PUSH(new Int(std::hash<string>()(self->sval)));
}

void str_contains() {
    String *other = (String *)POP_TYPE(str_type);
    if (other == NULL)
        {POP();return;}
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        return;
    if (self->sval.find(other->sval) != std::string::npos) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void str_startswith() {
    String *other = (String *)POP_TYPE(str_type);
    if (other == NULL)
        {POP();return;}
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        return;
    if (!self->sval.compare(0, other->sval.size(), other->sval)) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void str_split() {
    String *self = (String *)POP_TYPE(str_type);
    if (self == NULL)
        return;
    std::stringstream ss(self->sval);
    List *lst = new List();
    string part;
    while (std::getline(ss, part, ' ')) {
        lst->list->push_back(new String(part));
    }
    PUSH(lst);
}

void init_string() {
    str_type = new Class("str", NULL, 0);
    str_type->setmethod("__add__", str_add, 2);
    str_type->setmethod("__equals__", str_equals, 2);
    str_type->setmethod("__hash__", str_hash, 1);
    str_type->setmethod("__contains__", str_contains, 2);
    str_type->setmethod("split", str_split, 1);
    str_type->setmethod("startswith", str_startswith, 2);
}
