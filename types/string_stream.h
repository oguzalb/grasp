#ifndef GR_STRING_STREAM
#define GR_STRING_STREAM
#include <string>
#include <sstream>
#include "object.h"

class StringStream : public Object {
    public:
    stringstream ssval;
    StringStream();
};

void init_string_stream();

#endif
