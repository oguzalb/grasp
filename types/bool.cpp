#include "bool.h"
extern Class *bool_type;
extern Object *trueobject;
extern Object *falseobject;
extern std::unordered_map<string, Object *> *globals;

Bool::Bool(int bval) {
    this->type = bool_type;
    this->bval = bval;
}

void init_bool () {
    bool_type = new Class("bool", NULL, 0);
    trueobject = new Bool(TRUE);
    (*globals)["True"] = trueobject;
    falseobject = new Bool(FALSE);
    (*globals)["False"] = falseobject;
}
