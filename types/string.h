#ifndef GR_STR
#define GR_STR
#include <string>
#include "object.h"

class String : public Object {
    public:
    string sval;
    String(string sval);
};

void init_string();

#endif
