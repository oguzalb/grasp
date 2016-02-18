#include "object.h"
#include "class.h"
#include "../vm.h"

extern Class *object_type;

void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
}

void Object::setmethod(string name, void(*function)()) {
    BuiltinFunction *func = newbuiltinfunc_internal(function);
    this->setfield(name, func);
}

Object *Object::getfield(string name) {
// TODO exceptions
// TODO parent chain
    Object *field;
    try {
        field = this->fields.at(name);
    } catch (const std::out_of_range& oor) {
        assert(this->type);
        try {
            field = this->type->fields.at(name);
            cout << "getfield type " << field->type << endl;
        } catch (const std::out_of_range& oor) {
            cout << "no field, exception" << name << endl;
            newerror_internal("field not found");
            // TODO ok this is not the way it should work
            field = POP();
        }
    }
    return field;
}
