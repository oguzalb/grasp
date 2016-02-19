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
                field = type->fields.at(name);
                cout << "getfield type " << type << endl;
                break;
            } catch (const std::out_of_range& oor) {
            }
            cout << "no field" << name << " " << type->type_name << endl;
            type = type->type;
        }
    }
    return field;
}

void __str__() {
    Object *o = POP();
    PUSH(new String(string(o->type->type_name)));
}

void init_object() {
    object_type = new Class("Object", NULL);
    object_type->setmethod("__str__", __str__);
}
