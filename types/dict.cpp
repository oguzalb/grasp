#include "dict.h"
#include "../vm.h"

extern Class *dict_type;
extern std::vector<Object *> gstack;
extern Class *exception_type;
extern Class *str_type;
extern Class *int_type;
extern Class *func_type;
extern Class *class_type;
extern Class *builtinfunc_type;
extern Object *trueobject;
extern Object *none;
extern std::unordered_map<string, Object *> *globals;
extern Object *falseobject;

class Hasher
{
public:
  size_t operator() (Object *key) const
  {
    Object *hash_func = key->getfield("__hash__");
    if (hash_func == NULL) {
        //newerror_internal("does not have str", exception_type);
// TODO when dict is rewritten this should return an exception
        return false;
    }
    PUSH(hash_func);
    PUSH(key);
    assert(hash_func->type == func_type || hash_func->type == builtinfunc_type);
    Function *func = static_cast<Function *>(hash_func);
    call(1);
    Int *hash = POP_TYPE(Int, int_type);
    return std::hash<int>()(hash->ival);
  }
};

class EqualFn
{
public:
  bool operator() (Object *o1, Object *o2) const
  {
    Object *eq_func = o1->getfield("__equals__");
    if (eq_func == NULL) {
        //newerror_internal("does not have str", exception_type);
// TODO when dict is rewritten this should return an exception
        return false;
    }
    PUSH(eq_func);
    PUSH(o1);
    PUSH(o2);
    assert(eq_func->type == func_type || eq_func->type == builtinfunc_type);
    Function *func = static_cast<Function *>(eq_func);
    call(2);
    return POP() == trueobject;
  }
};

Dict::Dict() {
    this->type = dict_type;
    this->dict = new std::unordered_map<Object *, Object *, Hasher, EqualFn>();
}

void Dict::insert(Object *key, Object *val) {
    this->dict->insert({key, val});
}

void dict_str() {
    Dict *self = POP_TYPE(Dict, dict_type);
    string result = "{";
    for (auto i: *self->dict) {
        // TODO concurrency check later
        call_str(i.first);
        Object *exc = TOP();
        if (IS_EXCEPTION(exc))
            return;
        String *str_repr = POP_TYPE(String, str_type);
        result += str_repr->sval + ": ";
        call_str(i.second);
        exc = TOP();
        if (IS_EXCEPTION(exc))
            return;
        str_repr = POP_TYPE(String, str_type);
        result += str_repr->sval + ", ";
    }
    result += "}";
    PUSH(new String(result));
}

void dict_getitem() {
    Object *key = POP();
    Dict *self = POP_TYPE(Dict, dict_type);
    try {
        Object *val = self->dict->at(key);
        PUSH(val);
    } catch (const std::out_of_range& oor) {
        cerr << "no field in dict" << endl;
        newerror_internal("KeyError", exception_type);
   }
}

void dict_setitem() {
    Object *value = POP();
    Object *key = POP();
    Dict *self = POP_TYPE(Dict, dict_type);
    self->dict->insert({key, value});
    PUSH(none);
}

void dict_new() {
    Class *cls= POP_TYPE(Class, class_type);
    PUSH(new Dict());
}

void dict_contains() {
    Object *key = POP();
    Dict *self = POP_TYPE(Dict, dict_type);
    try {
        self->dict->at(key);
        PUSH(trueobject);
    } catch (const std::out_of_range& oor) {
        PUSH(falseobject);
   }
}

void init_dict() {
    dict_type = new Class("dict", dict_new, 1);
    dict_type->setmethod("__str__", dict_str, 1);
    dict_type->setmethod("__getitem__", dict_getitem, 2);
    dict_type->setmethod("__setitem__", dict_setitem, 3);
    dict_type->setmethod("__contains__", dict_contains, 2);
    (*globals)["dict"] = dict_type;
}
