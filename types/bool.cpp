#include "bool.h"
extern Class *bool_type;
extern Object *trueobject;
extern Object *falseobject;

Bool::Bool(int bval) {
    this->type = bool_type;
    this->bval = bval;
}

void init_bool () {
    bool_type = new Class("bool", NULL);
    trueobject = new Bool(TRUE);
    falseobject = new Bool(FALSE);
}
