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
#include "types/builtin_function.h"
#include "types/class.h"
#include "types/int.h"
#include "types/bool.h"
#include "types/string.h"
#include "types/function.h"
#include "types/list_iterator.h"
#include "types/list.h"

#define GETLOCAL(x) (gstack[bp+x])
#define GETFUNC() (gstack.at(bp-1))
#define LOCALSIZE() (gstack.size()-bp)
#define PUSH(x) gstack.push_back(x)
#define POP_TYPE(type, class_object) assert_type<type *>(POP(), class_object)
#define TOP() gstack.back()

#define TRUE 1
#define FALSE 0
#endif
