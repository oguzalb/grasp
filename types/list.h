#ifndef GR_LIST
#define GR_LIST
#include "object.h"
#include <vector>

class List : public Object {
    public:
    List();
    std::vector<Object *> *list;
};

#endif
