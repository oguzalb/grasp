#include "list_iterator.h"
#include "../vm.h"

extern Class *listiterator_type;
extern Class *stop_iteration_error;

ListIterator::ListIterator(std::vector<Object *> *list) {
    this->it = new std::vector<Object *>::iterator(list->begin());
cerr << "new iterator points to" << *(*this->it) << endl;
    this->end = new std::vector<Object *>::iterator(list->end());
}

void listiterator_next() {
    Object *self_obj = POP();
assert(self_obj->type == listiterator_type);
// TODO remove assert
    ListIterator *it_obj = static_cast<ListIterator *>(self_obj);
assert(it_obj->type == listiterator_type);
    Object *element;
    if (*it_obj->it != *it_obj->end) {
        element = static_cast<Object *>(**it_obj->it);
        (*it_obj->it)++;
        PUSH(element);
    } else {
        newerror_internal("STOP ITERATION!!!", stop_iteration_error);
    }
}

void init_listiterator() {
    listiterator_type = new Class("iterator", NULL, 0);
    listiterator_type->setmethod("next", listiterator_next, 1);
}
