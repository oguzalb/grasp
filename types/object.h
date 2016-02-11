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
    void setmethod(string name, void(*function)());
    Object *getfield(string name);
    Class *type;
};

#endif
