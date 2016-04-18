#include "object.h"
#include "class.h"
#include "../vm.h"

extern Class *object_type;
extern Class *class_type;
extern Class *str_type;
extern Object *none;

void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
}

void Object::setmethod(string name, void(*function)(), int param_count) {
    BuiltinFunction *func = new BuiltinFunction(function, param_count);
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
DEBUG_LOG(cerr << "searching " << name << " in " << type->type_name << endl;)
                field = type->fields.at(name);
DEBUG_LOG(cerr << "getfield type " << type->type_name << endl;)
                break;
            } catch (const std::out_of_range& oor) {
DEBUG_LOG(cerr << "no field" << name << " " << type->type_name << endl;)
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

void object_new() {
    Class *c = POP_TYPE(Class, class_type);
    Object *o = new Object();
    o->type = c;
    PUSH(o);
}

void object_getattr() {
    String *attr_name = POP_TYPE(String, str_type);
    Object *self = POP();
    Object *value;
    if (value = self->getfield(attr_name->sval)) {
        PUSH(value);
    } else {
        PUSH(none);
    }
}

void object_setattr() {
    Object *value = POP();
    String *attr_name = POP_TYPE(String, str_type);
    Object *self = POP();
    self->setfield(attr_name->sval, value);
    PUSH(none);
}

void init_object() {
    object_type = new Class("Object", NULL, 0);
    object_type->setmethod("__str__", __str__, 1);
    object_type->setmethod("__new__", object_new, 1);
    object_type->setmethod("__setattr__", object_setattr, 3);
    object_type->setmethod("__getattr__", object_getattr, 2);
}
