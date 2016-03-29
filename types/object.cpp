#include "object.h"
#include "class.h"
#include "../vm.h"

extern Class *object_type;

void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
}

void Object::setmethod(string name, void(*function)()) {
    BuiltinFunction *func = new BuiltinFunction(function);
    this->setfield(name, func);
}

Object *Object::getfield(string name) {
// TODO exceptions
// TODO parent chain
    Object *field = NULL;
    try {
        field = this->fields.at(name);
    } catch (const std::out_of_range& oor) {
        assert(this->type);
        Class *type = this->type;
        while (type != NULL) {
            try {
                cerr << "searching " << name << " in " << type->type_name << endl;
                field = type->fields.at(name);
                cerr << "getfield type " << type->type_name << endl;
                break;
            } catch (const std::out_of_range& oor) {
                cerr << "no field" << name << " " << type->type_name << endl;
            }
            type = type->type;
        }
    }
    return field;
}

int Object::isinstance(Object *type_to_check) {
    Object *type;
    Object *cls = this;
    while (cls != NULL) {
        if (cls->type == type_to_check)
            return TRUE;
        cls = cls->type;
    }
    return FALSE;
}

void __str__() {
    Object *o = POP();
    PUSH(new String(string("<") + string(o->type->type_name) + string(" object") + string(">")));
}

void init_object() {
    object_type = new Class("Object", NULL);
    object_type->setmethod("__str__", __str__);
}
