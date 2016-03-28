#ifndef GR_DICT
#define GR_DICT
#include "object.h"
#include <unordered_map>

class Dict : public Object {
    public:
    Dict();
    std::unordered_map<Object *, Object *> *dict;
};

void init_dict();

#endif
