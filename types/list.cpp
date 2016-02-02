#include "list.h"
extern Class *list_type;

List::List() {
    this->type = list_type;
    this->list = new std::vector<Object *>();
}