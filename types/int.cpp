#include "int.h"
#include "../vm.h"

extern Class *int_type;
extern Object *trueobject;
extern Object *falseobject;

Int::Int(int ival) {
    this->type = int_type;
    this->ival = ival;
}

void int_equals() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    if (o1->ival == o2->ival) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void int_hash() {
    Int *self = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(std::hash<int>()(self->ival)));
}

void int_add() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival + o2->ival));
}

void int_sub() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o2->ival - o1->ival));
}
 
void int_mul() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival * o2->ival));
}
 
void int_div() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival / o2->ival));
}

void init_int() {
    int_type = new Class("int", NULL, 0);
    int_type->setmethod("__add__", int_add, 2);
    int_type->setmethod("__sub__", int_sub, 2);
    int_type->setmethod("__mul__", int_mul, 2);
    int_type->setmethod("__div__", int_div, 2);
    int_type->setmethod("__equals__", int_equals, 2);
    int_type->setmethod("__hash__", int_hash, 1);
}
