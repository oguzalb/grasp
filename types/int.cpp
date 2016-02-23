#include "int.h"
#include "../vm.h"

extern Class *int_type;
extern Object *trueobject;
extern Object *falseobject;

Int::Int(int ival) {
    this->type = int_type;
    this->ival = ival;
}

void equals() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    if (o1->ival == o2->ival) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

void add() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival + o2->ival));
}

void sub() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o2->ival - o1->ival));
}
 
void mul() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival * o2->ival));
}
 
void div() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    PUSH(new Int(o1->ival / o2->ival));
}

void init_int() {
    int_type = new Class("int", NULL);
    int_type->setmethod("__add__", add);
    int_type->setmethod("__sub__", sub);
    int_type->setmethod("__mul__", mul);
    int_type->setmethod("__div__", div);
    int_type->setmethod("__equals__", equals);
}
