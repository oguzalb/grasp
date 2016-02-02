#include "list_iterator.h"
extern Class *listiterator_type;

ListIterator::ListIterator(std::vector<Object *> *list) {
    this->it = new std::vector<Object *>::iterator(list->begin());
cout << "new iterator points to" << *(*this->it) << endl;
    this->end = new std::vector<Object *>::iterator(list->end());
}