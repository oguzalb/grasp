#include "vm.h"
#include "assert.h"


unsigned int ip;
std::vector<Object *> gstack;
unsigned int bp;
std::vector<Object *> locals;
std::unordered_map<string, Object *> globals;
std::unordered_map<string, int> labels;

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
Class *object_type;

template<typename T> T assert_type(Object *o, Class *type)
{
    assert(o->type == type);
    return static_cast<T>(o);
}

inline Object *POP() {
    Object *o = gstack.back();
    gstack.pop_back();
    return o;
}

inline void PUSH(Object *x) {
    gstack.push_back(x);
}

void call(std::vector<std::string>& codes, int param_count);

void newerror_internal(string message) {
    Object *e = new Object();
    e->type = exception_type;
    String *m = new String(message);
    e->setfield("message", m);
    PUSH(e);
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
    cout << "__init__" << endl;
    }
    PUSH(o);
}

void newclass_internal() {
    Class *c = new Class("custom", newinstance);
    PUSH(c);
}

void newstr(string sval) {
    String *o = new String(sval);
    cout << "newstr: " << sval << endl;
    PUSH(o);
}

void newnone() {
    PUSH(none);
}

void newfunc(std::vector<std::string> &codes, int startp, string name) {
    Function *o = new Function(codes, startp, name);
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
cout << "field name type" << o1->type->type_name << endl;
    Object *o2 = POP();
cout << "object type" << o2->type->type_name << endl;
    Object *field = o2->getfield(o1->sval);
    if (field == NULL) {
        newerror_internal("Field not found");
        return;
    }
    PUSH(field);
    cout << "field pushed" << endl;
}

void getmethod() {
    String *o1 = POP_TYPE(String, str_type);
cout << "field name type:" << o1->type->type_name << endl;
    Object *o2 = TOP();
cout << "object type:" << o2->type->type_name << endl;
    Object *field = o2->getfield(o1->sval);
    cout << "type: " << o2->type->type_name << endl;
    if (field == NULL) {
        newerror_internal("Method not found");
        return;
    }
    assert(field->type == builtinfunc_type || field->type == func_type);
    PUSH(field);
    cout << "field pushed type " << field->type->type_name << endl;
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
    cout << "pushed type " << TOP()->type->type_name << endl;
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
    Object *result = TOP();
// stop iteration should have its type
    if (result->type != exception_type) {
        // continue, might get filled
    } else {
        ip = location;
        // pop the iterator and dummy none, not needed anymore
        assert(POP()->type == exception_type);
        assert(POP() == it);
    }
}

void swp() {
    Object *o1 = POP();
    Object *o2 = POP();
    PUSH(o1);
    PUSH(o2);
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

void call_str(Object *o) {
    Object *str_func = o->getfield("__str__");
    if (str_func == NULL) {
        newerror_internal("field not found");
        return;
    }
    PUSH(str_func);
    PUSH(o);
    assert(str_func->type == func_type || str_func->type == builtinfunc_type);
    Function *func = static_cast<Function *>(str_func);
    call(func->codes, 1);
}

void dummy () {
    // TODO something went wrong with compilation of this guy and i don't know why, will be fixed
    assert_type<Object *>(NULL, NULL);
}
void print_func() {
cout << "print" << endl;
    Object *o= POP();
    if (o->type == str_type)
        cout << assert_type<String *>(o, str_type)->sval << endl;
    else if (o->type == int_type)
        cout << assert_type<Int *>(o, int_type)->ival << endl;
    else {
        call_str(o);
        // TODO will be refactored
        String *str = POP_TYPE(String, str_type);
        cout << str->sval << endl;
    }
   PUSH(none);
}

void interpret_block(std::vector<std::string> &codes) {
    while (ip < codes.size()) {
        string command;
        string param;
        std::stringstream ss(codes[ip]);
        ss >> command;
        if (command == "pop") {
            print_func();
            POP();
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
            newfunc(codes, startp, name);
            cout << "next: " << codes[ip] << endl;
        } else if (command == "int") {
            int ival;
            ss >> ival;
// TODO check
            PUSH(new Int(ival));
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
// TODO check
        } else if (command == "class") {
            cout << "class " << endl;
            newclass_internal();
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
        if (gstack.size() > 0) {
            Object *exc = TOP();
            if (exc->type == exception_type) {
                break;
            }
        }
        ip++;
    }
}

void read_codes(std::stringstream& fs, std::vector<std::string> &codes) {
    std::string line;
    int index;
    int temp_ip = ip;
    // resets the labels
    labels = std::unordered_map<string, int>();
    while (std::getline(fs, line)) {
        if ((index = line.find(":")) != string::npos) {
cout << "label:" << line.substr(0, index) << " index:" << temp_ip << endl;
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

void print_stack_trace() {
    if (gstack.size() > 0) {
        Object *exc = POP();
        assert(exc->type == exception_type);
        PUSH(exc);
        print_func();
        POP();
    } else {
        cout << "no stack" << endl;
    }
}

void dump_codes(std::vector<std::string>& codes) {
    cout << "Dumping codes" << endl;
    int i = 0;
    for (auto &line : codes) {
        cout << i << " " << line << endl;
        i++;
    }
    cout << "Dumping labels" << endl;
    for (auto &label : labels) {
        cout << label.first << ":" << label.second << endl;
    }
}


void init_builtins() {
    init_builtin_func();
    // TODO new instance functions should be implemented
    init_object();
    init_bool();
    class_type = new Class("Class", NULL);
    class_type->type = object_type;
    init_exception();
    init_int();
    init_function();
    init_string();
    init_list();
    init_listiterator();
    BuiltinFunction *range = new BuiltinFunction(range_func);
    globals["range"] = range;
    BuiltinFunction *print = new BuiltinFunction(print_func);
    globals["print"] = print;
    none_type = new Class("NoneType", NULL);
    none = new Object();
    none->type = none_type;
    globals["None"] = none;
}

bool ends_with(const string& s, const string& ending) {
 return (s.size() >= ending.size()) && std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}
