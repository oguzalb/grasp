#include "list.h"
#include "../vm.h"

extern Class *list_type;
extern Class *listiterator_type;

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

void init_list() {
    list_type = new Class("list", NULL);
    list_type->setmethod("iter", list_iter);
}
