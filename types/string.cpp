#include "string.h"
extern Class *str_type;

String::String(string sval) {
    this->type = str_type;
    this->sval = sval;
}