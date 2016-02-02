#include "int.h"
extern Class *int_type;

Int::Int(int ival) {
    this->type = int_type;
    this->ival = ival;
}