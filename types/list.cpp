#include "list.h"
#include "../vm.h"

extern Class *list_type;
extern Class *listiterator_type;
extern std::vector<Object *> gstack;
extern Class *exception_type;
extern Class *str_type;
extern Class *int_type;
extern Class *class_type;
extern std::unordered_map<string, Object*> *globals;
extern Object *none;

List::List() {
    this->type = list_type;
    this->list = new std::vector<Object *>();
}

void list_iter() {
    Object *self_obj = POP();
// TODO remove assert
    assert(self_obj->type == list_type);
    List *self = static_cast<List *>(self_obj);
    ListIterator *it_obj = new ListIterator(self->list);
    it_obj->type = listiterator_type;
    PUSH(it_obj);
}

void list_str() {
    List *self = POP_TYPE(List, list_type);
    string result = "[";
    for (int i=0; i< self->list->size(); i++) {
        // TODO concurrency check later
        // sbuff = )
        call_str(self->list->at(i));
        Object *exc = TOP();
        if (IS_EXCEPTION(exc))
            return;
        String *str_repr = POP_TYPE(String, str_type);
        result += str_repr->sval + ", ";
    }
    result += "]";
    PUSH(new String(result));
}

void list_len() {
    List *self = POP_TYPE(List, list_type);
    PUSH(new Int(self->list->size()));
}

void list_append() {
    Object *value = POP();
    List *self = POP_TYPE(List, list_type);
    self->list->push_back(value);
    PUSH(none);
}

void list_new() {
    Class *cls = POP_TYPE(Class, class_type);
    PUSH(new List());
}

void list_getitem() {
    Int *index = POP_TYPE(Int, int_type);
    List *self = POP_TYPE(List, list_type);
    try {
        PUSH(self->list->at(index->ival));
    } catch (const std::out_of_range& oor) {
        newerror_internal("IndexError", exception_type);
    }
}

void init_list() {
    list_type = new Class("list", list_new, 1);
    list_type->setmethod("iter", list_iter, 1);
    list_type->setmethod("__str__", list_str, 1);
    list_type->setmethod("__len__", list_len, 1);
    list_type->setmethod("__getitem__", list_getitem, 2);
    list_type->setmethod("append", list_append, 2);
    (*globals)["list"] = list_type;
}
