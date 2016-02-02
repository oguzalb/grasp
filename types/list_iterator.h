#ifndef GR_LIST_ITERATOR
#define GR_LIST_ITERATOR
#include "object.h"
#include <vector>

class ListIterator : public Object {
    public:
    ListIterator(std::vector<Object *> *);
    std::vector<Object *>::iterator *it;
    std::vector<Object *>::iterator *end;
};

#endif
