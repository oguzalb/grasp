#include "class.h"

extern Class *class_type;
extern Class *object_type;

Class::Class(string type_name, void(*function)()) : BuiltinFunction(function){
    this->type_name = type_name;
    this->type = class_type;
}
