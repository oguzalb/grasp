#ifndef GR_OBJECT
#define GR_OBJECT
#include <unordered_map>
#include <iostream>
#include <string>
#include "assert.h"

using namespace std;
class Class;
class Object {
    public:
    std::unordered_map<string, Object *> fields;
    void setfield(string name, Object* object);
    void setmethod(string name, void(*function)(), int param_count);
    Object *getfield(string name);
    int isinstance(Object *type);
    Class *type;
};

void init_object();
void object_new();
#endif
