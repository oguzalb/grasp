#include "dict.h"
#include "../vm.h"

extern Class *dict_type;
extern std::vector<Object *> gstack;
extern Object *exception_type;
extern Class *str_type;

Dict::Dict() {
    this->type = dict_type;
    this->dict = new std::unordered_map<Object *, Object *>();
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

void init_dict() {
    dict_type = new Class("dict", NULL);
    dict_type->setmethod("__str__", dict_str);
}
