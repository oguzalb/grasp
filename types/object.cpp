#include "object.h"
#include "class.h"

extern Class *object_type;

void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
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
        } catch (const std::out_of_range& oor) {
            return NULL;
        }
    }
    cout << "getfield type " << field->type << endl;
    return field;
}
