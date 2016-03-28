#include "list.h"
#include "../vm.h"

extern Class *list_type;
extern Class *listiterator_type;
extern std::vector<Object *> gstack;
extern Object *exception_type;
extern Class *str_type;

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

void init_list() {
    list_type = new Class("list", NULL);
    list_type->setmethod("iter", list_iter);
    list_type->setmethod("__str__", list_str);
}
