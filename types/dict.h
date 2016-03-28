#ifndef GR_DICT
#define GR_DICT
#include "object.h"
#include <unordered_map>

class Hasher;

class EqualFn;

class Dict : public Object {
    public:
    Dict();
    std::unordered_map<Object *, Object *, Hasher, EqualFn> *dict;
    void insert(Object *key, Object *val);
};

void init_dict();

#endif
