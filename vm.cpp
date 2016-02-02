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

#define GETLOCAL(x) (gstack[bp+x])
#define GETFUNC() (gstack.at(bp-1))
#define LOCALSIZE() (gstack.size()-bp)

template<typename T> T assert_type(Object * o, Class *type)
{
    assert(o->type == type);
    return static_cast<T>(o);
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
        gstack.push_back(f);
        gstack.push_back(o);
        cout << "localsize:" << localsize << endl;
        for (int i=0; i<localsize; i++)
            gstack.push_back(GETLOCAL(i));
        call(f->codes, 1 + localsize);
        gstack.pop_back();
        gstack.pop_back();
    }
cout << "__init__" << endl;
    gstack.push_back(o);
}

void newclass_internal() {
    Class *c = new Class("custom");
    c->function = newinstance;
    gstack.push_back(c);
}

void newint(int ival) {
    Object *o = new Int(ival);
    gstack.push_back(o);
}

void newstr(string sval) {
    String *o = new String(sval);
    cout << "newstr: " << sval << endl;
    gstack.push_back(o);
}

void newnone() {
    gstack.push_back(none);
}

void newfunc(std::vector<std::string> &codes, int startp) {
    Function *o = new Function;
    o->type = func_type;
    o->codep = startp;
    o->codes = codes;
    gstack.push_back(o);
}

void setfield() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    // TODO exception
    assert(o2->type == str_type);
    String *s = static_cast<String *>(o2);
    gstack.pop_back();
    Object *o3 = gstack.back();
    gstack.pop_back();
    o3->setfield(s->sval, o1);
    cout << "field set: " << s->sval << endl;
}

void getfield() {
    String *o1 = assert_type<String *>(gstack.back(), str_type);
cout << "object type" << o1->type << endl;
    gstack.pop_back();
    Object *o2 = gstack.back();
cout << "object type" << o2->type << endl;
    gstack.pop_back();
    Object *field =o2->getfield(o1->sval);
    if (field == NULL)
        throw exception();
    gstack.push_back(field);
    cout << "field pushed" << endl;
}

void getmethod() {
    String *o1 = assert_type<String *>(gstack.back(), str_type);
cout << "object type" << o1->type << endl;
    gstack.pop_back();
    Object *o2 = gstack.back();
cout << "object type" << o2->type << endl;
    Object *field = o2->getfield(o1->sval);
    if (field == NULL)
        throw exception();
    assert(field->type == builtinfunc_type || field->type == func_type);
    gstack.push_back(field);
    cout << "field pushed type " << field->type << endl;
}

void setglobal(string name) {
    globals[name] = gstack.back();
    gstack.pop_back();
}

void setlocal(unsigned int ival) {
    if (ival >= (gstack.size() - bp)) {
        cerr << "OUPSSS, size bigger than locals" << endl;
        exit(1);
    }
// must be improved for local defs
    Object *oval = gstack.back();
    gstack[bp + ival] = oval;
    gstack.pop_back();
}

void pushglobal(string name) {
    try {
        gstack.push_back(globals.at(name));
    } catch (const std::out_of_range& oor) {
        cerr << "Global named " << name << " not found" << endl;
        exit(1);
    }
}

void pushlocal(unsigned int ival) {
    if (ival >= gstack.size() - bp) {
        cerr << "OUPSSS, size bigger than locals" << endl;
        exit(1);
    }
    cout << "pushed type " << gstack[bp + ival]->type << endl;
    gstack.push_back(GETLOCAL(ival));
}


void interpret_block(std::vector<std::string>& codes);

void call(std::vector<std::string>& codes, int param_count) {
    // TODO stuff about param_count
    bp = gstack.size() - param_count;

    Object *callable = GETFUNC();
    // TODO exc
    if (callable->type == func_type) {
        Function *func = static_cast<Function *>(callable);
        int cur_ip = ip;
        ip = func->codep;
        interpret_block(codes);
        Object *result = gstack.back();
        gstack.pop_back();
        gstack.resize(gstack.size() - param_count);
        func = static_cast<Function *>(gstack.back());
        cout << func->type->type_name << endl;
        assert(func->type == func_type);
        gstack.pop_back();
        gstack.push_back(result);
        ip = cur_ip;
    } else if (callable->type == builtinfunc_type || callable->type == class_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        func->function();
        Object *result = gstack.back();
        gstack.pop_back();
// builtin funcs consume the parameters
        assert(gstack.back()->type == builtinfunc_type || gstack.back()->type == class_type);
        gstack.pop_back();
        gstack.push_back(result);
    } else {
        cout << callable->type << endl;
        assert(FALSE);
    }
} 

void swp();
void loop(std::vector<std::string>& codes, int location) {
    Object *it = gstack.back();
    assert(gstack.back() == it);
// next will consume this, should be optimized later on
    gstack.push_back(it);
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
        assert(gstack.back() == none);
        gstack.pop_back();
        assert(gstack.back() == it);
        gstack.pop_back();
    }
}

void add() {
    Int *o1 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    Int *o2 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    // TODO exc
    newint(o1->ival + o2->ival);
}
 
void sub() {
    Int *o1 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    Int *o2 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    // TODO exc
    newint(o2->ival - o1->ival);
}
 
void mul() {
    Int *o1 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    Int *o2 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    // TODO exc
    newint(o1->ival * o2->ival);
}
 
void div() {
    Int *o1 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    Int *o2 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    // TODO exc
    newint(o2->ival / o1->ival);
}

void swp() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    gstack.pop_back();
    gstack.push_back(o1);
    gstack.push_back(o2);
}

void equals() {
    Int *o1 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    Int *o2 = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    if (o1->ival == o2->ival) {
        gstack.push_back(trueobject);
    } else {
        gstack.push_back(falseobject);
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
            Object *val = gstack.back();
            if (val->type == int_type)
                cout << "popped " << assert_type<Int *>(val, int_type)->ival << endl;
            else if (val->type == none_type)
                cout << "popped none" << endl;
            else if (val->type == func_type)
                cout << "popped func" << endl;
            gstack.pop_back();
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
        } else if (command == "add") {
            cout << "add" << endl;
            add();
        } else if (command == "sub") {
            cout << "sub" << endl;
            sub();
        } else if (command == "mul") {
            cout << "mul" << endl;
            mul();
        } else if (command == "div") {
            cout << "div" << endl;
            div();
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
            Object *o = gstack.back();
            gstack.pop_back();
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
    int ip = 0;
    while (std::getline(fs, line)) {
        if ((index = line.find(":"))) {
            labels.insert({line.substr(0, index), ip});
            line = line.substr(index+1);
        }
        codes.push_back(line);
        ip++;
    }
}

void range_func() {
    Int *max = assert_type<Int *>(gstack.back(), int_type);
    gstack.pop_back();
    List *list = new List();
    for (int i=0; i<max->ival; i++)
        list->list->push_back(new Int(i));
    gstack.push_back(list);
}

void print_func() {
cout << "print" << endl;
    Object *o = gstack.back();
    gstack.pop_back();
    if (o->type == str_type)
        {cout << "asdadsasd" << endl;cout << assert_type<String *>(o, str_type)->sval << endl;}
    else if (o->type == int_type)
        cout << assert_type<Int *>(o, int_type)->ival << endl;
    else
        assert(FALSE);
    gstack.push_back(none);
}

void list_iter() {
    Object *self_obj = gstack.back();
// TODO remove assert
    assert(self_obj->type == list_type);
    List *self = static_cast<List *>(self_obj);
    gstack.pop_back();
    ListIterator *it_obj = new ListIterator(self->list);
    it_obj->type = listiterator_type;
    gstack.push_back(it_obj);
}

void listiterator_next() {
    Object *self_obj = gstack.back();
assert(self_obj->type == listiterator_type);
// TODO remove assert
    ListIterator *it_obj = static_cast<ListIterator *>(self_obj);
assert(it_obj->type == listiterator_type);
    gstack.pop_back();
    Object *element;
    if (*it_obj->it != *it_obj->end) {
        element = static_cast<Object *>(**it_obj->it);
        (*it_obj->it)++;
        gstack.push_back(element);
    } else {
        gstack.push_back(none);
        // TODO when exception types implemented
        newerror_internal("STOP ITERATION!!!");
    }
}

BuiltinFunction *newbuiltinfunc_internal (void(*function)()) {
    BuiltinFunction *func = new BuiltinFunction();
    func->type = builtinfunc_type;
    func->function = function;
    return func;
}

void init_builtins() {
    error = NULL;
    bool_type = new Class("bool");
    trueobject = newbool_internal(TRUE);
    falseobject = newbool_internal(FALSE);
    exception_type = new Class("exception");
    class_type = new Class("class");
    int_type = new Class("int");
    func_type = new Class("func");
    none_type = new Class("none");
    str_type = new Class("str");
    list_type = new Class("list");
    builtinfunc_type = new Class("builtin_func");
    BuiltinFunction *range = newbuiltinfunc_internal(range_func);
    globals["range"] = range;
    BuiltinFunction *iter_func = newbuiltinfunc_internal(list_iter);
    list_type->setfield("iter", iter_func);
    listiterator_type = new Class("iterator");
    BuiltinFunction *next_func = newbuiltinfunc_internal(listiterator_next);
    listiterator_type->setfield("next", next_func);
    BuiltinFunction *print = newbuiltinfunc_internal(print_func);
    globals["print"] = print;
    none_type = new Class("none");
    none = new Object();
    globals["none"] = none;
    none->type = none_type;
}

int main () {
    init_builtins();
    std::fstream fs;
    fs.open("test.graspo", std::fstream::in);
    std::stringstream ss;
    std::vector<std::string> codes;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    read_codes(ss, codes);
    ip = 0;
    interpret_block(codes);
    return 0;
}
