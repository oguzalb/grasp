#ifndef JS_VM_H
#define JS_VM_H
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <unordered_map>
#include <stdexcept>
using namespace std;
class Object {
    public:
    int type;
    string sval;
    int ival;
    std::stringstream *code;
};
std::vector<Object *> gstack;
int bp;
std::vector<Object *> locals;
std::unordered_map<string, Object *> globals;
#define TRUE 1
#define FALSE 0
#endif
