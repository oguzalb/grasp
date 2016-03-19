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
#include "types/object.h"
#include "types/module.h"
#include "types/builtin_function.h"
#include "types/class.h"
#include "types/int.h"
#include "types/bool.h"
#include "types/string.h"
#include "types/function.h"
#include "types/list_iterator.h"
#include "types/list.h"
#include "types/exception.h"

#define GETLOCAL(x) (gstack[bp+x])
#define GETFUNC() (gstack.at(bp-1))
#define LOCALSIZE() (gstack.size()-bp)
#define POP_TYPE(type, class_object) assert_type<type *>(POP(), class_object)
#define TOP() gstack.back()
#define IS_EXCEPTION(type) type->isinstance(exception_type)


#define TRUE 1
#define FALSE 0

void init_builtins(std::vector<string> *codes);
void interpret_block(std::vector<std::string>& codes);
void convert_codes(std::stringstream& fs, std::vector<std::string> &codes);
bool ends_with(const string& s, const string& ending);
BuiltinFunction *newbuiltinfunc_internal(void(*function)());
inline Object* POP();
inline void PUSH(Object *);
template<typename T> T assert_type(Object *o, Class *type);
void newerror_internal(string message, Class *type);
void print_stack_trace();
void dump_codes(std::vector<std::string>& codes);
std::stringstream read_codes(string filename);
void dump_stack();
void compile_file(string module_name);
void setglobal(string name);
void newinstance();
#endif
