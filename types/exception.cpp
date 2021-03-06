#include "exception.h"
#include "../vm.h"

extern Class *exception_type;

void exc_str() {
    Object *exc = POP_TYPE(exception_type);
    if (exc == NULL)
        return;
    Object *str = exc->getfield("message");
    if (str == NULL) {
        newerror_internal("Exception should have message field", exception_type);
        return;
    }
    PUSH(str);
}

void init_exception() {
    exception_type = new Class("Exception", object_new, 0);
    exception_type->setmethod("__str__", exc_str, 1);
    PUSH(exception_type);
    setglobal("Exception");
}
