#include "class.h"

extern Class *class_type;
extern Class *object_type;

Class::Class(string type_name, void(*function)(), int param_count) : BuiltinFunction(function, param_count){
    this->type_name = type_name;
    this->type = class_type;
}
