#include "exception.h"
#include "../vm.h"

extern Class *exception_type;

void exc_str() {
    Object *exc = POP_TYPE(Object, exception_type);
    Object *str = exc->getfield("message");
    if (str == NULL) {
        newerror_internal("Exception should have message field", exception_type);
        return;
    }
    PUSH(str);
}

void init_exception() {
    exception_type = new Class("Exception", newinstance);
    exception_type->setmethod("__str__", exc_str);
    PUSH(exception_type);
    setglobal("Exception");
}
