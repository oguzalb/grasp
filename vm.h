#ifndef GR_VM_H
#define GR_VM_H
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
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
#include "types/dict.h"
#include "types/string_stream.h"
#include "types/exception.h"

#include "modules/grmysql.h"

#define GETLOCAL(x) (gstack[bp+x])
#define GETFUNC() (gstack.at(bp-1))
#define LOCALSIZE() (gstack.size()-bp)
#define POP_TYPE(type, class_object) (assert_type<type *>(POP(), class_object))
#define TOP() gstack.back()
#define IS_EXCEPTION(type) (type->isinstance(exception_type))
#ifdef DEBUG
#define DEBUG_LOG(x) x
#else
#define DEBUG_LOG(x) ;;
#endif

#ifdef CALC_FLAG
#define CALC(x) x
#else
#define CALC(x) ;;
#endif

#define TRUE 1
#define FALSE 0

void init_builtins(std::vector<unsigned char> *codes, int argc, char *argv[], char *env[]);
void interpret_block(std::vector<Object *>* co_consts, std::vector<unsigned char>& codes);
void convert_codes(std::stringstream& fs, std::vector<unsigned char> &codes);
bool ends_with(const string& s, const string& ending);
BuiltinFunction *newbuiltinfunc_internal(void(*function)());
inline Object* POP();
inline void PUSH(Object *);
template<typename T> T assert_type(Object *o, Class *type);
void newerror_internal(string message, Class *type);
void print_stack_trace();
void dump_codes(std::vector<unsigned char>& codes);
std::stringstream *read_codes(string filename);
void dump_stack();
void compile_file(string module_name);
void setglobal(string name);
void newinstance();
void call_str(Object *o);
void print_func();
void call(int count);
void dump_counters();
#endif
