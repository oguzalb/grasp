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
string main_path;
string stack_machine_path;
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
Class *string_stream_type;

bool repl = false;

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

void call_init() {
    Object *instance = POP();
// TODO CHECK
    Function* f = static_cast<Function *>(instance->getfield("__init__"));
    int localsize = LOCALSIZE();
    PUSH(f);
    if (f != NULL) {
        PUSH(instance);
        for (int i=0; i<localsize; i++)
            PUSH(GETLOCAL(i));
        cerr << "calling __init__ with" << localsize + 1 << endl;
        call(f->codes, localsize + 1);
        Object *result = TOP();
        if (IS_EXCEPTION(result))
            return;
    }
    Object *init_result = POP();
    // consume __new__ params that are passed to init
    gstack.resize(gstack.size() - localsize);
    PUSH(init_result);
}

void newclass_internal() {
    Class *c = new Class("custom", object_new, -1);
    PUSH(c);
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void newstr(string sval) {
    replace_all(sval, "\\n", "\n");
    String *o = new String(sval);
    cerr << "newstr: " << sval << endl;
    PUSH(o);
}

void newnone() {
    PUSH(none);
}

void newfunc(std::unordered_map<string, Object *> *globals, std::vector<std::string> &codes, int startp, string name, int locals_count, int param_count) {
    Function *o = new Function(globals, codes, startp, name, locals_count, param_count);
    PUSH(o);
}

void setfield() {
    Object *o1 = POP();
    // TODO exception
    String *s = POP_TYPE(String, str_type);
    Object *o3 = POP();
    o3->setfield(s->sval, o1);
    cerr << "field set: " << s->sval << endl;
}

void getfield() {
    String *o1 = POP_TYPE(String, str_type);
cerr << "field name type" << o1->type->type_name << endl;
    Object *o2 = POP();
cerr << "object type" << o2->type->type_name << endl;
    Object *field = o2->getfield(o1->sval);
    if (field == NULL) {
        newerror_internal("Field not found",exception_type);
        return;
    }
    PUSH(field);
    cerr << "field pushed" << endl;
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
cerr << "field name type:" << o1->type->type_name << endl;
    Object *o2 = POP();
cerr << "object type:" << o2->type->type_name << endl;
    Object *field = o2->getfield(o1->sval);
    cerr << "type: " << o2->type->type_name << endl;
    if (field == NULL) {
        newerror_internal("Method not found", exception_type);
        return;
    }
    assert(field->type == builtinfunc_type || field->type == func_type);
    PUSH(field);
    PUSH(o2);
    cerr << "field pushed type " << field->type->type_name << endl;
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
    string extension = ".graspo";
    string file_full_path = main_path + "/" + module_name + extension;
    std::stringstream *ss = read_codes(file_full_path);
    if (ss == NULL) {
        return NULL;
    }
    Module *module = new Module(new std::vector<string>());
    convert_codes(*ss, *module->codes);
    delete ss;
    int tmp_ip = ip;
    std::unordered_map<string, Object *> *globals_tmp = globals;
    globals = &module->fields;
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
    cerr << "IMPORTING " << module_name << "." << var_name << endl;
    Object *module;
        cerr << "before load" << module_name<< ":" <<ip << endl;
    if (imported_modules.find(module_name) == imported_modules.end()) {
        module = load_module(module_name);
        if (module == NULL)
            return;
        imported_modules[module_name] = module;
    } else {
        cerr << "before at" << endl;
        module = imported_modules.at(module_name);
        cerr << "after at" << endl;
    }
        cerr << "after load" << module_name<< ":" <<ip << endl;
    assert(module->type == module_type);
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
    cerr << "pushed type " << TOP()->type->type_name << endl;
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
    Function *new_func;
    // TODO exc
    if (callable->type == func_type) {
// TODO
        Function *func = static_cast<Function *>(callable);
        if (func->param_count != -1 && func->param_count != param_count) {
            std::ostringstream ss;
            ss << "Expected" << func->param_count << " got " << param_count;
            newerror_internal(ss.str().c_str(), exception_type);
            return;
        }
        int cur_ip = ip;
        ip = func->codep;
        gstack.resize(gstack.size() + func->locals_count, NULL);
        std::unordered_map<string, Object *> *temp_globals = globals;
        globals = func->globals;
        interpret_block(func->codes);
        globals = temp_globals;
        Object *result = POP();
        gstack.resize(gstack.size() - (param_count + func->locals_count));
        func = POP_TYPE(Function, func_type);
        PUSH(result);
        ip = cur_ip;
    } else if (callable->type == builtinfunc_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        if (func->param_count != -1 && func->param_count != param_count) {
            std::ostringstream ss;
            ss << "Expected " << func->param_count << "parameters, got " << param_count << endl;
            newerror_internal(ss.str().c_str(), exception_type);
            return;
        }
        func->function();
        Object *result = POP();
        cerr << "returned " << result->type->type_name << endl;
// builtin funcs consume the parameters
        BuiltinFunction* func_after = static_cast<BuiltinFunction *>(POP());
        assert(func_after == func);
        PUSH(result);
    } else if (callable->type == class_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        // param_count + 1 because of the cls variable!
        if (func->param_count != -1 && func->param_count != param_count + 1) {
            std::ostringstream ss;
            ss << "Expected" << func->param_count << " got " << param_count + 1;
            newerror_internal(ss.str().c_str(), exception_type);
            return;
        }
        if (func->function == NULL) {
            newerror_internal("Class does not have builtin __new__!!!", exception_type);
            return;
        }
        // we need to push the type of class that we will get instance of
        PUSH(func);
        func->function();
        Object *instance = POP();
        // push the instance for init to use
        cerr << "returned " << instance->type->type_name << endl;
        PUSH(instance);
        call_init();
        if (gstack.size() > 0 && IS_EXCEPTION(TOP()))
            return;
        // pop none
        POP();
        BuiltinFunction* func_after = static_cast<BuiltinFunction *>(POP());
        // return the instance
        PUSH(instance);
    } else if (new_func = static_cast<Function *>(callable->getfield("__new__"))) {
// TODO this section is never tested!!!
        if (new_func == NULL) {
            newerror_internal("__new__ not found!!!", exception_type);
            return;
        }
        call(new_func->codes, param_count);
        call_init();
    } else {
        cerr << callable->type->type_name << endl;
        assert(FALSE);
    }
    delete traps;
    traps = tmp_traps;
    bp = bp_temp;
    int size_after = gstack.size();
    //cerr << size_before << ":" << size_after + param_count << endl;
    assert(size_before == size_after + param_count);
} 

void swp();
void loop(std::vector<std::string>& codes, int location) {
    Object *it = TOP();
// next will consume this, should be optimized later on
    PUSH(it);
    newstr("next");
// TODO CHECK EXCEPTIONS!!!
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
    cerr << "swapping " <<  o1->type->type_name << " with " << o2->type->type_name << endl;
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
    POP_TYPE(Class, NULL);
    POP_TYPE(MysqlConnection, NULL);
    POP_TYPE(StringStream, NULL);
    POP_TYPE(Bool, NULL);
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
    cerr << "bp: " << bp << endl;
    for (int i=0; i < gstack.size(); i++) {
        Object *o = gstack.at(i);
        if (o == NULL)
            cerr << "UNBOUND" << endl;
        else
            cerr << i << ": " << o->type->type_name << endl;
    }
}

void interpret_block(std::vector<std::string> &codes) {
    int block_bp = gstack.size() - 1;
    while (ip < codes.size()) {
        string command;
        string param;
        std::stringstream ss(codes[ip]);
        ss >> command;
        if (command == "pop") {
            if (TOP()->type != none_type && repl)
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
            int param_count;
            ss >> param_count;
            // TODO sanity check
            cerr << "function code read " << startp << " param_count:" << param_count<<endl;
// TODO
            newfunc(globals, codes, ip + startp, name, locals_count, param_count);
            cerr << "next: " << codes[ip+startp] << endl;
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
            cerr << "call " << count <<endl;
            call(codes, count);
// TODO check
        } else if (command == "class") {
            cerr << "class " << endl;
            newclass_internal();
// TODO check
        } else if (command == "import") {
            cerr << "import" << endl;
            string module_name;
            ss >> module_name;
            string var_name;
            ss >> var_name;
            import(module_name, var_name);
        } else if (command == "return") {
            cerr << "return" << endl;
            break;
// TODO check
        } else if (command == "pushlocal") {
            unsigned int lindex;
            ss >> lindex;
// TODO check
            cerr << "pushlocal " << lindex << endl;
            pushlocal(lindex);
        } else if (command == "pushglobal") {
            string name;
            ss >> name;
// TODO check
            cerr << "pushglobal " << name << endl;
            pushglobal(name);
        } else if (command == "setlocal") {
            int lindex;
            ss >> lindex;
            cerr << "setlocal " << lindex << endl;
            setlocal(lindex);
        } else if (command == "setglobal") {
            string name;
            ss >> name;
            cerr << "setglobal " << name << endl;
            setglobal(name);
        } else if (command == "swp") {
            cerr << "swp" << endl;
            swp();
        } else if (command == "nop") {
            cerr << "nop" << endl;
        } else if (command == "getfield") {
            cerr << "getfield" << endl;
            getfield();
        } else if (command == "setfield") {
            cerr << "setfield" << endl;
            setfield();
        } else if (command == "getmethod") {
            cerr << "getmethod" << endl;
            getmethod();
        } else if (command == "jmp") {
            int location;
            ss >> location;
// TODO check
            cerr << "jmp " << location << endl;
            ip += location-1; // will increase at the end of loop
        } else if (command == "pop_trap_jmp") {
            traps->pop_back();
// TODO this instruction will be reconsidered
            int location;
            ss >> location;
// TODO check
            cerr << "pop_trap_jmp " << location << endl;
            ip += location-1; // will increase at the end of loop
        } else if (command == "loop"){
            int location;
            ss >> location;
            cerr << "loop " << location << endl;
            loop(codes, location);
        } else if (command == "trap"){
            int location;
            ss >> location;
            cerr << "trap " << location << endl;
            trap(location);
        } else if (command == "onerr"){
            int location;
            ss >> location;
            cerr << "onerr " << location << endl;
            onerr(location);
        } else if (command == "jnt") {
            int location;
            ss >> location;
// TODO check
            cerr << "jnt " << location << endl;
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
                POP();
                gstack.resize(block_bp + 1);
                PUSH(exc);
                break;
            }
        }
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
        cerr << "no stack to print" << endl;
    }
}

void convert_codes(std::stringstream& fs, std::vector<std::string> &codes) {
    std::string line;
    while (std::getline(fs, line)) {
        codes.push_back(line);
    }
}

void dump_codes(std::vector<std::string>& codes) {
    cerr << "Dumping codes" << endl;
    int i = 0;
    for (auto &line : codes) {
        cerr << i << " " << line << endl;
        i++;
    }
}

std::stringstream *read_codes(string filename) {
    std::fstream fs;
    fs.open(filename, std::fstream::in);
    if (!fs.is_open()) {
        newerror_internal("File could not be opened: " + filename, exception_type);
        return NULL;
    }
    std::stringstream *ss = new std::stringstream;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(*ss));
    return ss;
}
 
void init_builtins(std::vector<std::string> *codes, int argc, char *argv[], char *env[]) {
    main_module = new Module(codes);
    globals = &main_module->fields;
    traps = new std::vector<int>();
    init_builtin_func();
    // TODO new instance functions should be implemented
    init_object();
    class_type = new Class("Class", NULL, -1);
    class_type->type = object_type;
    init_module();
    init_bool();
    init_exception();
    stop_iteration_error = new Class("StopIterationError", NULL, 0);
    stop_iteration_error->type = exception_type;
    (*globals)["StopIterationError"] = stop_iteration_error;
    assertion_error = new Class("AssertionError", NULL, 0);
    assertion_error->type = exception_type;
    (*globals)["AssertionError"] = assertion_error;
    init_int();
    init_function();
    init_string();
    (*globals)["str"] = str_type;
    init_string_stream();
    (*globals)["StringStream"] = string_stream_type;
    init_list();
    init_dict();
    init_listiterator();
    BuiltinFunction *range = new BuiltinFunction(range_func, 1);
    (*globals)["range"] = range;
    BuiltinFunction *isinstance = new BuiltinFunction(isinstance_func, 2);
    (*globals)["isinstance"] = isinstance;
    BuiltinFunction *print = new BuiltinFunction(print_func, 1);
    (*globals)["print"] = print;
    BuiltinFunction *assert = new BuiltinFunction(assert_func, 1);
    (*globals)["assert"] = assert;
    none_type = new Class("NoneType", NULL, 0);
    none = new Object();
    none->type = none_type;
    (*globals)["None"] = none;
    builtins = globals;
    globals = new std::unordered_map<string, Object *>();
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
        cerr << "parsing envvar: " << *env << endl;
        std::cerr.flush();
        char *saveptr;
        char *key = strtok_r(*env, "=", &saveptr);
        if (key == NULL)
            continue;
        char *val = strtok_r(NULL, "\0", &saveptr);
        if (val == NULL)
            val = "";
        o_env->insert(new String(key), new String(val));
        env++;
    }
    sys_module->setfield("env", o_env);
    init_grmysql();
}

void compile_file(string module_name) {
 // TODO workaround, will be changed
 // when grasp starts compiling itself
   FILE *fpipe;
   string command=string("python ") + stack_machine_path + "/grasp.py ";
// TODO check if it was already there
   string compile_command = command + main_path + "/" + module_name + ".grasp";
   cerr << "compile command for " << module_name << ": " << compile_command << endl;
   if (!(fpipe = (FILE*)popen(compile_command.c_str(), "r")) ) {
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
