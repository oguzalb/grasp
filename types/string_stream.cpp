#include "string.h"
#include "../vm.h"

extern Class *str_type;
extern Class *class_type;
extern Class *string_stream_type;
extern std::unordered_map<string, Object*> *globals;

StringStream::StringStream() {
    this->type = string_stream_type;
}

void strstr_append() {
    Object *other = POP();
    StringStream *self = (StringStream *)POP_TYPE(string_stream_type);
    if (self == NULL)
        return;
    if (other->type == str_type) {
        String *other_string = static_cast<String *>(other);
        self->ssval << other_string->sval;
    } else if (other->type == string_stream_type) {
        StringStream *other_string_stream = static_cast<StringStream *>(other);
        self->ssval << other_string_stream->ssval.str();
    }
    PUSH(self);
}

void strstr_new() {
    Class *cls = (Class *)POP_TYPE(class_type);
    if (cls == NULL)
        return;
    PUSH(new StringStream());
}

void strstr_str() {
    StringStream *self = (StringStream *)POP_TYPE(string_stream_type);
    if (self == NULL)
        return;
    PUSH(new String(self->ssval.str()));
}

void init_string_stream() {
    string_stream_type = new Class("StringStream", strstr_new, 1);
    string_stream_type->setmethod("append", strstr_append, 2);
    string_stream_type->setmethod("__str__", strstr_str, 1);
    (*globals)["StringStream"] = string_stream_type;
}
