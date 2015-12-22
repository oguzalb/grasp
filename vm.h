#ifndef GR_VM_H
#define GR_VM_H
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <unordered_map>
#include <stdexcept>
using namespace std;
class Function;
class Object {
    public:
    string sval;
    int ival;
    std::unordered_map<string, Object *> fields;
    void setfield(string name, Object* object);
    Object *getfield(string name);
    Object *type;
};

class Function : public Object {
    public:
    int codep;
};

class BuiltinFunction : public Object {
    public:
    void (*function) ();
};

class Bool : public Object {
    public:
    int bval;
};

class List : public Object {
    public:
    std::vector<Object *> *list;
};

class ListIterator : public Object {
    public:
    ListIterator(std::vector<Object *> *);
    std::vector<Object *>::iterator *it;
    std::vector<Object *>::iterator *end;
};

int ip;
std::vector<Object *> gstack;
int bp;
std::vector<Object *> locals;
std::unordered_map<string, Object *> globals;
std::unordered_map<string, int> labels;
Object *error;
#define TRUE 1
#define FALSE 0
#endif
