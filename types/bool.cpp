#include "bool.h"
extern Class *bool_type;
extern Object *trueobject;
extern Object *falseobject;
extern std::unordered_map<string, Object *> *globals;

Bool::Bool(int bval) {
    this->type = bool_type;
    this->bval = bval;
}

void bool_str() {
    Object *o = POP_TYPE(Bool, bool_type);
    string str = (o == trueobject? "True": "False");
    PUSH(new String(str));
}

void init_bool () {
    bool_type = new Class("bool", NULL, 0);
    bool_type->setmethod("__str__", bool_str, 1);
    trueobject = new Bool(TRUE);
    (*globals)["True"] = trueobject;
    falseobject = new Bool(FALSE);
    (*globals)["False"] = falseobject;
}
