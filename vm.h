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
    int type;
    string sval;
    int ival;
    std::unordered_map<string, Object *> fields;
    void setfield(string name, Object* object);
    Object *getfield(string name);
};

class Function : public Object {
    public:
    int codep;
};

class Bool : public Object {
    public:
    int bval;
};

int ip;
std::vector<Object *> gstack;
int bp;
std::vector<Object *> locals;
std::unordered_map<string, Object *> globals;
std::unordered_map<string, int> labels;
#define TRUE 1
#define FALSE 0
#endif
