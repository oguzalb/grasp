#include "vm.h"
#include "assert.h"


unsigned int ip;
std::vector<Object *> gstack;
unsigned int bp;
std::vector<Object *> locals;
std::unordered_map<string, Object *> globals;
std::unordered_map<string, int> labels;
Object *error;

Bool *trueobject;
Bool *falseobject;
Object *none;
Class *func_type;
Class *builtinfunc_type;
Class *none_type;
Class *bool_type;
Class *str_type;
Class *list_type;
Class *listiterator_type;
Class *exception_type;
Class *class_type;
Class *int_type;

template<typename T> T assert_type(Object * o, Class *type)
{
    assert(o->type == type);
    return static_cast<T>(o);
}

inline Object *POP() {
    Object *o = gstack.back();
    gstack.pop_back();
    return o;
}

void call(std::vector<std::string>& codes, int param_count);

Bool *newbool_internal(int bval) {
    Bool *o = new Bool;
    o->type = bool_type;
    o->bval = bval;
    cout << "newbool: " << bval << endl;
    return o;
}

void newerror_internal(string message) {
    Object *e = new Object();
    e->type = exception_type;
    String *m = new String(message);
    e->setfield("message", m);
    error = e;
}

void newinstance() {
    Object *co = GETFUNC();
    assert(co->type == class_type);
    Class *c = static_cast<Class *>(co);
    Object *o = new Object();
    o->type = c;
cout << "newinstance" << endl;
// TODO CHECK
    Function* f = static_cast<Function *>(o->getfield("__init__"));
    if (f != NULL) {
        int localsize = LOCALSIZE();
        PUSH(f);
        PUSH(o);
        cout << "localsize:" << localsize << endl;
        for (int i=0; i<localsize; i++)
            PUSH(GETLOCAL(i));
        call(f->codes, 1 + localsize);
        POP();
        POP();
    }
cout << "__init__" << endl;
    PUSH(o);
}

void newclass_internal() {
    Class *c = new Class("custom", newinstance);
    PUSH(c);
}

void newint(int ival) {
    Object *o = new Int(ival);
    PUSH(o);
}

void newstr(string sval) {
    String *o = new String(sval);
    cout << "newstr: " << sval << endl;
    PUSH(o);
}

void newnone() {
    PUSH(none);
}

void newfunc(std::vector<std::string> &codes, int startp) {
    Function *o = new Function(codes, startp);
    PUSH(o);
}

void setfield() {
    Object *o1 = POP();
    // TODO exception
    String *s = POP_TYPE(String, str_type);
    Object *o3 = POP();
    o3->setfield(s->sval, o1);
    cout << "field set: " << s->sval << endl;
}

void getfield() {
    String *o1 = POP_TYPE(String, str_type);
cout << "object type" << o1->type << endl;
    Object *o2 = POP();
cout << "object type" << o2->type << endl;
    Object *field =o2->getfield(o1->sval);
    if (field == NULL)
        throw exception();
    PUSH(field);
    cout << "field pushed" << endl;
}

void getmethod() {
    String *o1 = POP_TYPE(String, str_type);
cout << "object type:" << o1->type << endl;
    Object *o2 = TOP();
cout << "object type:" << o2->type << endl;
    Object *field = o2->getfield(o1->sval);
    cout << "type: " << o2->type->type_name << endl;
    if (field == NULL) {
        throw exception();
    }
    assert(field->type == builtinfunc_type || field->type == func_type);
    PUSH(field);
    cout << "field pushed type " << field->type << endl;
}

void setglobal(string name) {
    globals[name] = POP();
}

void setlocal(unsigned int ival) {
    if (ival >= (LOCALSIZE())) {
        cerr << "OUPSSS, size bigger than locals" << endl;
        exit(1);
    }
// must be improved for local defs
    Object *oval = POP();
    gstack[bp + ival] = oval;
}

void pushglobal(string name) {
    try {
        PUSH(globals.at(name));
    } catch (const std::out_of_range& oor) {
        cerr << "Global named " << name << " not found" << endl;
        exit(1);
    }
}

void pushlocal(unsigned int ival) {
    if (ival >= LOCALSIZE()) {
        cerr << "OUPSSS, size bigger than locals" << endl;
        exit(1);
    }
    gstack.push_back(GETLOCAL(ival));
    cout << "pushed type " << TOP() << endl;
}


void interpret_block(std::vector<std::string>& codes);

void call(std::vector<std::string>& codes, int param_count) {
    // TODO stuff about param_count
    bp = gstack.size() - param_count;

    Object *callable = GETFUNC();
    // TODO exc
    if (callable->type == func_type) {
// TODO
        Function *func = static_cast<Function *>(callable);
        int cur_ip = ip;
        ip = func->codep;
        interpret_block(codes);
        Object *result = POP();
        gstack.resize(gstack.size() - param_count);
        func = POP_TYPE(Function, func_type);
        PUSH(result);
        ip = cur_ip;
    } else if (callable->type == builtinfunc_type || callable->type == class_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        func->function();
        Object *result = POP();
// builtin funcs consume the parameters
        BuiltinFunction* func_after = static_cast<BuiltinFunction *>(POP());
        assert(func_after == func);
        PUSH(result);
    } else {
        cout << callable->type << endl;
        assert(FALSE);
    }
} 

void swp();
void loop(std::vector<std::string>& codes, int location) {
    Object *it = TOP();
// next will consume this, should be optimized later on
    PUSH(it);
    newstr("next");
    getmethod();
    swp();
    call(codes, 1);
// stop iteration should have its type
    if (error == NULL) {
        // continue, might get filled
    } else {
        error = NULL;
        ip = location;
        // pop the iterator and dummy none, not needed anymore
        assert(POP() == none);
        assert(POP() == it);
    }
}

void add() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    newint(o1->ival + o2->ival);
}
 
void sub() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    newint(o2->ival - o1->ival);
}
 
void mul() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    newint(o1->ival * o2->ival);
}
 
void div() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    // TODO exc
    newint(o2->ival / o1->ival);
}

void swp() {
    Object *o1 = POP();
    Object *o2 = POP();
    PUSH(o1);
    PUSH(o2);
}

void equals() {
    Int *o1 = POP_TYPE(Int, int_type);
    Int *o2 = POP_TYPE(Int, int_type);
    if (o1->ival == o2->ival) {
        PUSH(trueobject);
    } else {
        PUSH(falseobject);
    }
}

std::vector<std::string> *read_func_code(std::vector<std::string> &codes) {
    std::stringstream ss;
    ss << "endfunction";
    std::string endcommand = ss.str();
    std::vector<std::string> *funccode = new std::vector<std::string>;
    string line;
    while (ip < codes.size() && (line = codes[ip]) != endcommand) {
        funccode->push_back(line);
        ip++;
    }
    return funccode;
}

void interpret_block(std::vector<std::string> &codes) {
    while (ip < codes.size()) {
        string command;
        string param;
        std::stringstream ss(codes[ip]);
        ss >> command;
        if (command == "pop") {
            Object *val = POP();
            if (val->type == int_type)
                cout << "popped " << assert_type<Int *>(val, int_type)->ival << endl;
            else if (val->type == none_type)
                cout << "popped none" << endl;
            else if (val->type == func_type)
                cout << "popped func" << endl;
        } else if (command == "function") {
            string name;
// TODO check
            int startp;
            string startlabel;
            ss >> startlabel;
            // TODO sanity check
            startp = labels.at(startlabel);
            cout << "function code read " << startp << endl;
// TODO
            newfunc(codes, startp);
            cout << "next: " << codes[ip] << endl;
        } else if (command == "int") {
            int ival;
            ss >> ival;
// TODO check
            newint(ival);
        } else if (command == "str") {
            string sval = ss.str().substr(4);
// TODO check
            newstr(sval);
        } else if (command == "dup") {
            gstack.push_back(gstack.back());
        } else if (command == "call") {
            int count;
            ss >> count;
            cout << "call " << count <<endl;
            call(codes, count);
            if (error != NULL)
                throw exception();
// TODO check
        } else if (command == "class") {
            cout << "class " << endl;
            newclass_internal();
            if (error != NULL)
                throw exception();
// TODO check
 
        } else if (command == "return") {
            cout << "return" << endl;
            break;
// TODO check
        } else if (command == "pushlocal") {
            unsigned int lindex;
            ss >> lindex;
// TODO check
            cout << "pushlocal " << lindex << endl;
            pushlocal(lindex);
        } else if (command == "pushglobal") {
            string name;
            ss >> name;
// TODO check
            cout << "pushglobal " << name << endl;
            pushglobal(name);
        } else if (command == "setlocal") {
            int lindex;
            ss >> lindex;
            cout << "setlocal " << lindex << endl;
            setlocal(lindex);
        } else if (command == "setglobal") {
            string name;
            ss >> name;
            cout << "setglobal " << name << endl;
            setglobal(name);
        } else if (command == "swp") {
            cout << "swp" << endl;
            swp();
        } else if (command == "nop") {
            cout << "nop" << endl;
        } else if (command == "equals") {
            cout << "equals" << endl;
            equals();
        } else if (command == "getfield") {
            cout << "getfield" << endl;
            getfield();
        } else if (command == "setfield") {
            cout << "setfield" << endl;
            setfield();
        } else if (command == "getmethod") {
            cout << "getmethod" << endl;
            getmethod();
        } else if (command == "jmp") {
            string label;
            ss >> label;
            int location = labels.at(label);
// TODO check
            cout << "jmp " << location << endl;
            ip = location-1; // will increase at the end of loop
        } else if (command == "loop"){
            string label;
            ss >> label;
            int location = labels.at(label);
            cout << "loop " << location << endl;
            loop(codes, location);
        } else if (command == "jnt") {
            string label;
            ss >> label;
            int location = labels.at(label);
// TODO check
            cout << "jnt " << location << endl;
            Object *o = POP();
            if (o == falseobject)
                ip = location-1; // will increase at the end of loop
        } else {
            cerr << "command not defined" << command << endl;
            throw std::exception();
        }
        ip++;
    }
}

void read_codes(std::stringstream& fs, std::vector<std::string> &codes) {
    std::string line;
    int index;
    int temp_ip = ip;
    while (std::getline(fs, line)) {
        if ((index = line.find(":"))) {
            labels.insert({line.substr(0, index), temp_ip});
            line = line.substr(index+1);
        }
        codes.push_back(line);
        temp_ip++;
    }
}

void range_func() {
    Int *max = POP_TYPE(Int, int_type);
    List *list = new List();
    for (int i=0; i<max->ival; i++)
        list->list->push_back(new Int(i));
    PUSH(list);
}

void print_func() {
cout << "print" << endl;
    Object *o = POP();
    if (o->type == str_type)
        cout << assert_type<String *>(o, str_type)->sval << endl;
    else if (o->type == int_type)
        cout << assert_type<Int *>(o, int_type)->ival << endl;
    else
        assert(FALSE);
    PUSH(none);
}

void list_iter() {
    Object *self_obj = POP();
// TODO remove assert
    assert(self_obj->type == list_type);
    List *self = static_cast<List *>(self_obj);
    ListIterator *it_obj = new ListIterator(self->list);
    it_obj->type = listiterator_type;
    PUSH(it_obj);
}

void listiterator_next() {
    Object *self_obj = POP();
assert(self_obj->type == listiterator_type);
// TODO remove assert
    ListIterator *it_obj = static_cast<ListIterator *>(self_obj);
assert(it_obj->type == listiterator_type);
    Object *element;
    if (*it_obj->it != *it_obj->end) {
        element = static_cast<Object *>(**it_obj->it);
        (*it_obj->it)++;
        PUSH(element);
    } else {
        PUSH(none);
        // TODO when exception types implemented
        newerror_internal("STOP ITERATION!!!");
    }
}

BuiltinFunction *newbuiltinfunc_internal (void(*function)()) {
    BuiltinFunction *func = new BuiltinFunction(function);
    return func;
}

void init_builtins() {
    error = NULL;
    // TODO new instance functions should be implemented
    builtinfunc_type = new Class("builtin_func", NULL);
    bool_type = new Class("bool", NULL);
    trueobject = newbool_internal(TRUE);
    falseobject = newbool_internal(FALSE);
    exception_type = new Class("exception", NULL);
    class_type = new Class("class", NULL);
    int_type = new Class("int", NULL);
    int_type->setmethod("__add__", add);
    func_type = new Class("func", NULL);
    none_type = new Class("none", NULL);
    str_type = new Class("str", NULL);
    list_type = new Class("list", NULL);
    BuiltinFunction *range = newbuiltinfunc_internal(range_func);
    globals["range"] = range;
    list_type->setmethod("iter", list_iter);
    listiterator_type = new Class("iterator", NULL);
    listiterator_type->setmethod("next", listiterator_next);
    BuiltinFunction *print = newbuiltinfunc_internal(print_func);
    globals["print"] = print;
    none_type = new Class("none", NULL);
    none = new Object();
    globals["none"] = none;
    none->type = none_type;
}

bool ends_with(const string& s, const string& ending) {
 return (s.size() >= ending.size()) && std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}
