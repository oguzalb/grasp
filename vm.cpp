#include "vm.h"
#include "assert.h"

unsigned int ip;
std::vector<Object *> gstack;
unsigned int bp;
std::vector<Object *> locals;
std::vector<int> *traps;
std::unordered_map<string, Object *> *builtins;
std::unordered_map<string, Object *> *globals;
std::unordered_map<string, Object *> imported_modules;
Module *main_module;
Bool *trueobject;
Bool *falseobject;
Object *none;
Class *assertion_error;
Class *stop_iteration_error;
Class *func_type;
Class *builtinfunc_type;
Class *none_type;
Class *bool_type;
Class *str_type;
Class *list_type;
Class *dict_type;
Class *listiterator_type;
Class *exception_type;
Class *class_type;
Class *int_type;
Class *object_type;
Class *module_type;

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

void newerror_internal(string message, Class *type) {
    Object *e = new Object();
    e->type = type;
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

void newfunc(std::vector<std::string> &codes, int startp, string name, int locals_count) {
    Function *o = new Function(codes, startp, name, locals_count);
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
        newerror_internal("Field not found",exception_type);
        return;
    }
    PUSH(field);
    cout << "field pushed" << endl;
}

void isinstance_func() {
    Object *type = POP();
    Object *o = POP();
    if (o->isinstance(type) == TRUE)
        PUSH(trueobject);
    else
        PUSH(falseobject);
}

void assert_func() {
    Object *result = POP_TYPE(Object, bool_type);
    if (result == trueobject)
        PUSH(none);
    else {
        newerror_internal("Assert failed", assertion_error);
    }
}

void getmethod() {
    String *o1 = POP_TYPE(String, str_type);
cout << "field name type:" << o1->type->type_name << endl;
    Object *o2 = POP();
cout << "object type:" << o2->type->type_name << endl;
    Object *field = o2->getfield(o1->sval);
    cout << "type: " << o2->type->type_name << endl;
    if (field == NULL) {
        newerror_internal("Method not found", exception_type);
        return;
    }
    assert(field->type == builtinfunc_type || field->type == func_type);
    PUSH(field);
    PUSH(o2);
    cout << "field pushed type " << field->type->type_name << endl;
}

void setglobal(string name) {
    (*globals)[name] = POP();
}

inline Object *getglobal(string name) {
    Object *o = NULL;
    try {
        o = globals->at(name);
    } catch (const std::out_of_range& oor) {
        try {
            o = builtins->at(name);
        } catch (const std::out_of_range& oor) {
            cerr << "Global named " << name << " not found in builtins" << endl;
            return NULL;
        }
    }
    return o;
}

Object *load_module(string module_name) {
    compile_file(module_name);
    std::stringstream ss = read_codes(module_name + string(".graspo"));
    Module *module = new Module(new std::vector<string>());
    std::unordered_map<string, Object *> *globals_tmp = globals;
    globals = &module->fields;
    convert_codes(ss, *module->codes);
    int tmp_ip = ip;
    ip = 0;
    interpret_block(*module->codes);
    ip = tmp_ip;
    globals = globals_tmp;
    if (gstack.size() > 0) {
        if (IS_EXCEPTION(TOP()))
            return NULL;
    }
    //dump_stack();
    return module;
}

void import(string module_name, string var_name) {
    cout << module_name << "." << var_name << endl;
    Object *module;
    if (imported_modules.find(module_name) == imported_modules.end()) {
        module = load_module(module_name);
        if (module == NULL)
            return;
    } else {
        module = imported_modules.at(var_name);
    }
    assert(module->type == module_type);
    imported_modules[module_name] = module;
    Object *var = module->getfield(var_name);
    if (var == NULL) {
        string message = "Couldn't import " + var_name + " from " + module_name;
        newerror_internal(message, exception_type);
        return;
    }
    (*globals)[var_name] = module->getfield(var_name);
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
    Object *o = getglobal(name);
    if (o == NULL)
        exit(1);
    PUSH(o);
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

void trap(int location) {
    traps->push_back(ip+location);
}

void onerr(int location) {
    // TODO if it is assignment we have a problem!!!
    Object* o = POP();
    if (!IS_EXCEPTION(o)) {
        ip += location - 1;
    }
    // TODO set the err
}

void call(std::vector<std::string>& codes, int param_count) {
    // TODO stuff about param_count
    int size_before = gstack.size();
    int bp_temp = bp;
    std::vector<int> *tmp_traps = traps;
    std::vector<int> *traps = new std::vector<int>();
    bp = gstack.size() - param_count;
    Object *callable = GETFUNC();
    // TODO exc
    if (callable->type == func_type) {
// TODO
        Function *func = static_cast<Function *>(callable);
        int cur_ip = ip;
        ip = func->codep;
        gstack.resize(gstack.size() + func->locals_count, NULL);
        interpret_block(func->codes);
        Object *result = POP();
        gstack.resize(gstack.size() - (param_count + func->locals_count));
        func = POP_TYPE(Function, func_type);
        PUSH(result);
        ip = cur_ip;
    } else if (callable->type == builtinfunc_type || callable->type == class_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        func->function();
        Object *result = POP();
        cout << "returned " << result->type->type_name << endl;
// builtin funcs consume the parameters
        BuiltinFunction* func_after = static_cast<BuiltinFunction *>(POP());
        assert(func_after == func);
        PUSH(result);
    } else {
        cout << callable->type->type_name << endl;
        assert(FALSE);
    }
    delete traps;
    traps = tmp_traps;
    bp = bp_temp;
    int size_after = gstack.size();
    //cout << size_before << ":" << size_after + param_count << endl;
    assert(size_before == size_after + param_count);
} 

void swp();
void loop(std::vector<std::string>& codes, int location) {
    Object *it = TOP();
// next will consume this, should be optimized later on
    PUSH(it);
    newstr("next");
    getmethod();
    call(codes, 1);
    Object *result = TOP();
// stop iteration should have its type
    if (!result->isinstance(stop_iteration_error)) {
        // continue, might get filled
    } else {
        ip += location;
        // pop the exception and iterator, not needed anymore
        POP();
        assert(POP() == it);
    }
}

void swp() {
    Object *o1 = POP();
    Object *o2 = POP();
    cout << "swapping " <<  o1->type->type_name << " with " << o2->type->type_name << endl;
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
    if (o->type == str_type) {
        PUSH(o);
        return;
    }
    Object *str_func = o->getfield("__str__");
    if (str_func == NULL) {
        newerror_internal("does not have str", exception_type);
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
    POP_TYPE(String, str_type);
    POP_TYPE(List, NULL);
    POP_TYPE(Dict, NULL);
}
void print_func() {
    Object *o= POP();
    if (o->type == str_type)
        cout << assert_type<String *>(o, str_type)->sval << endl;
    else if (o->type == int_type)
        cout << assert_type<Int *>(o, int_type)->ival << endl;
    else {
        call_str(o);
        if (IS_EXCEPTION(TOP())) {
            return;
        }
        // TODO will be refactored
        String *str = POP_TYPE(String, str_type);
        cout << str->sval << endl;
    }
   PUSH(none);
}

void dump_stack() {
    cout << "bp: " << bp << endl;
    for (int i=0; i < gstack.size(); i++) {
        Object *o = gstack.at(i);
        if (o == NULL)
            cout << "UNBOUND" << endl;
        cout << o->type->type_name << endl;
    }
}

void interpret_block(std::vector<std::string> &codes) {
    while (ip < codes.size()) {
        string command;
        string param;
        std::stringstream ss(codes[ip]);
        ss >> command;
        if (command == "pop") {
            if (TOP()->type != none_type)
                print_func();
            if (!IS_EXCEPTION(TOP()))
                POP();
        } else if (command == "function") {
            string name;
// TODO check
            int startp;
            ss >> startp;
            int locals_count;
            ss >> locals_count;
            // TODO sanity check
            cout << "function code read " << startp << endl;
// TODO
            newfunc(codes, ip + startp, name, locals_count);
            cout << "next: " << codes[ip+startp] << endl;
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
        } else if (command == "import") {
            cout << "import" << endl;
            string module_name;
            ss >> module_name;
            string var_name;
            ss >> var_name;
            import(module_name, var_name);
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
            int location;
            ss >> location;
// TODO check
            cout << "jmp " << location << endl;
            ip += location-1; // will increase at the end of loop
        } else if (command == "pop_trap_jmp") {
            traps->pop_back();
// TODO this instruction will be reconsidered
            int location;
            ss >> location;
// TODO check
            cout << "pop_trap_jmp " << location << endl;
            ip += location-1; // will increase at the end of loop
        } else if (command == "loop"){
            int location;
            ss >> location;
            cout << "loop " << location << endl;
            loop(codes, location);
        } else if (command == "trap"){
            int location;
            ss >> location;
            cout << "trap " << location << endl;
            trap(location);
        } else if (command == "onerr"){
            int location;
            ss >> location;
            cout << "onerr " << location << endl;
            onerr(location);
        } else if (command == "jnt") {
            int location;
            ss >> location;
// TODO check
            cout << "jnt " << location << endl;
            Object *o = POP();
            if (o == falseobject)
                ip += location - 1; // will increase at the end of loop
        } else {
            cerr << "command not defined" << command << endl;
            throw std::exception();
        }
        if (gstack.size() > 0) {
            Object *exc = TOP();
            if (IS_EXCEPTION(exc)) {
                if (traps->size() > 0) {
                    int location = traps->back();
                    traps->pop_back();
                    ip = location-1;
                    continue;
                }
                break;
            }
        }
        //dump_stack();
        ip++;
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
        assert(IS_EXCEPTION(exc));
        PUSH(exc);
        print_func();
        POP();
    } else {
        cout << "no stack to print" << endl;
    }
}

void convert_codes(std::stringstream& fs, std::vector<std::string> &codes) {
    std::string line;
    int index;
    int ip_start = codes.size();
    int temp_ip = ip;
    while (std::getline(fs, line)) {
        if ((index = line.find(":")) != string::npos) {
cout << "label:" << line.substr(0, index) << " index:" << temp_ip << endl;
            line = line.substr(index+1);
        }
        codes.push_back(line);
        temp_ip++;
    }
    ip = ip_start;
}

void dump_codes(std::vector<std::string>& codes) {
    cout << "Dumping codes" << endl;
    int i = 0;
    for (auto &line : codes) {
        cout << i << " " << line << endl;
        i++;
    }
}

std::stringstream read_codes(string filename) {
    std::fstream fs;
    fs.open(filename, std::fstream::in);
    std::stringstream ss;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    return ss;
}
 
void init_builtins(std::vector<std::string> *codes, int argc, char *argv[], char *env[]) {
    globals = new std::unordered_map<string, Object *>();
    traps = new std::vector<int>();
    init_builtin_func();
    // TODO new instance functions should be implemented
    init_object();
    class_type = new Class("Class", NULL);
    class_type->type = object_type;
    init_bool();
    init_exception();
    stop_iteration_error = new Class("StopIterationError", NULL);
    stop_iteration_error->type = exception_type;
    (*globals)["StopIterationError"] = stop_iteration_error;
    assertion_error = new Class("AssertionError", NULL);
    assertion_error->type = exception_type;
    (*globals)["AssertionError"] = assertion_error;
    init_int();
    init_function();
    init_string();
    (*globals)["str"] = str_type;
    init_list();
    init_dict();
    init_listiterator();
    BuiltinFunction *range = new BuiltinFunction(range_func);
    (*globals)["range"] = range;
    BuiltinFunction *isinstance = new BuiltinFunction(isinstance_func);
    (*globals)["isinstance"] = isinstance;
    BuiltinFunction *print = new BuiltinFunction(print_func);
    (*globals)["print"] = print;
    BuiltinFunction *assert = new BuiltinFunction(assert_func);
    (*globals)["assert"] = assert;
    none_type = new Class("NoneType", NULL);
    none = new Object();
    none->type = none_type;
    (*globals)["None"] = none;
    builtins = globals;
    globals = new std::unordered_map<string, Object *>();
    init_module();
    main_module = new Module(codes);
    Module *sys_module = new Module(NULL);
    (*globals)["sys"] = sys_module;
    Int *o_argc = new Int(argc);
    sys_module->setfield("argc", o_argc);
    List *o_argv = new List();
    for (int i=0; i < argc; i++)
        o_argv->list->push_back(new String(argv[i]));
    sys_module->setfield("argv", o_argv);
    Dict *o_env = new Dict();
    while (*env != NULL) {
        char *saveptr;
        char *key = strtok_r(*env, "=", &saveptr);
        char *val = strtok_r(NULL, "\0", &saveptr);
        if (val == NULL)
            val = "";
        o_env->insert(new String(key), new String(val));
        env++;
    }
    sys_module->setfield("env", o_env);
    
}

void compile_file(string module_name) {
 // TODO workaround, will be changed
 // when grasp starts compiling itself
   FILE *fpipe;
   string command="python grasp.py ";
// TODO check if it was already there
   const char *compile_command = (command + module_name + ".grasp").c_str();
   if (!(fpipe = (FILE*)popen(compile_command, "r")) ) {
      perror("Problems with pipe");
      exit(1);
   }
    char buff[1024];
    while(fgets(buff, sizeof(buff), fpipe)!=NULL) {
        printf("%s", buff);
    }

// TODO error check
   pclose(fpipe);
}

bool ends_with(const string& s, const string& ending) {
 return (s.size() >= ending.size()) && std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}
