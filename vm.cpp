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


#define I_STR 1

#define I_POP 10
#define I_DUP 11
#define I_RETURN 12
#define I_SWP 13
#define I_NOP 14
#define I_CLASS 15
#define I_SETITEM 16
#define I_GETMETHOD 17
#define I_SETFIELD 18
#define I_GETFIELD 19
#define I_CALL 20
#define I_PUSHLOCAL 21
#define I_PUSHGLOBAL 22
#define I_SETLOCAL 23
#define I_SETGLOBAL 24
#define I_JMP 25
#define I_POP_TRAP_JMP 26
#define I_LOOP 27
#define I_TRAP 28
#define I_ONERR 29
#define I_JNT 30
#define I_PUSHCONST 31
#define I_BUILD_LIST 32
#define I_BUILD_DICT 33
#define I_INT 34

#define I_IMPORT 50

#define I_FUNCTION 61

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

void call(int param_count);

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
DEBUG_LOG(cerr << "calling __init__ with" << localsize + 1 << endl;)
        call(localsize + 1);
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
    String *o = new String(sval);
DEBUG_LOG(cerr << "newstr: " << sval << endl;)
    PUSH(o);
}

void build_list(int count) {
    List *list = new List();
    for (int i=0; i<count;i++) {
        list->list->push_back(POP());
    }
    PUSH(list);
}

void build_dict(int count) {
    Dict *dict = new Dict();
    for (int i=0; i<count;i++) {
        Object *val = POP();
        Object *key = POP();
        dict->insert(key, val);
    }
    PUSH(dict);
}

void newnone() {
    PUSH(none);
}

void newfunc(std::unordered_map<string, Object *> *globals, std::vector<Object *> *co_consts, std::vector<unsigned char> &codes, int startp, string name, int locals_count, int param_count) {
    Function *o = new Function(globals, co_consts, codes, startp, name, locals_count, param_count);
    PUSH(o);
}

void setfield() {
    Object *o1 = POP();
    // TODO exception
    String *s = POP_TYPE(String, str_type);
    Object *o3 = POP();
    o3->setfield(s->sval, o1);
DEBUG_LOG(cerr << "field set: " << s->sval << endl;)
}

void setitem() {
    Object *val = POP();
    Object *key = POP();
    Object *dict = POP();
    Object *func = dict->getfield("__setitem__");
    if (func == NULL) {
        newerror_internal("__setitem__ not found", exception_type);
        return;
    }
    PUSH(func);
    PUSH(dict);
    PUSH(val);
    PUSH(key);
    call(3);
    POP();
DEBUG_LOG(cerr << "item set: " << endl;)
}

void getfield() {
    String *o1 = POP_TYPE(String, str_type);
DEBUG_LOG(cerr << "field name type" << o1->type->type_name << endl;)
    Object *o2 = POP();
DEBUG_LOG(cerr << "object type" << o2->type->type_name << endl;)
    Object *field = o2->getfield(o1->sval);
    if (field == NULL) {
        newerror_internal("Field not found",exception_type);
        return;
    }
    PUSH(field);
DEBUG_LOG(cerr << "field pushed" << endl;)
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
DEBUG_LOG(cerr << "field name type:" << o1->type->type_name << endl;)
    Object *o2 = POP();
DEBUG_LOG(cerr << "object type:" << o2->type->type_name << endl;)
    Object *field = o2->getfield(o1->sval);
DEBUG_LOG(cerr << "type: " << o2->type->type_name << endl;)
    if (field == NULL) {
        newerror_internal("Method not found", exception_type);
        return;
    }
    assert(field->type == builtinfunc_type || field->type == func_type);
    PUSH(field);
    PUSH(o2);
DEBUG_LOG(cerr << "field pushed type " << field->type->type_name << endl;)
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
DEBUG_LOG(cerr << "Global named " << name << " not found in builtins" << endl;)
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
    std::vector<Object *> *co_consts = new std::vector<Object *>();
    co_consts->push_back(none);
    Module *module = new Module(co_consts, new std::vector<unsigned char>);
    module->co_consts = co_consts;
    convert_codes(*ss, *module->codes);
    delete ss;
    int tmp_ip = ip;
    std::unordered_map<string, Object *> *globals_tmp = globals;
    globals = &module->fields;
    ip = 0;
    interpret_block(module->co_consts, *module->codes);
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
DEBUG_LOG(cerr << "IMPORTING " << module_name << "." << var_name << endl;)
    Object *module;
DEBUG_LOG(cerr << "before load" << module_name<< ":" <<ip << endl;)
    if (imported_modules.find(module_name) == imported_modules.end()) {
        module = load_module(module_name);
        if (module == NULL)
            return;
        imported_modules[module_name] = module;
    } else {
DEBUG_LOG(cerr << "before at" << endl;)
        module = imported_modules.at(module_name);
DEBUG_LOG(cerr << "after at" << endl;)
    }
DEBUG_LOG(cerr << "after load" << module_name<< ":" <<ip << endl;)
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
DEBUG_LOG(cerr << "OUPSSS, size bigger than locals" << endl;)
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

void pushlocal(int ival) {
    if (ival >= LOCALSIZE()) {
DEBUG_LOG(cerr << "OUPSSS, size bigger than locals" << endl;)
        exit(1);
    }
    gstack.push_back(GETLOCAL(ival));
DEBUG_LOG(cerr << "pushed type " << TOP()->type->type_name << endl;)
}


void interpret_block(std::vector<Object *>*co_consts, std::vector<unsigned char>& codes);

void trap(int location) {
    traps->push_back(ip+location);
}

void onerr(int location) {
    // TODO if it is assignment we have a problem!!!
    Object* o = POP();
    if (!IS_EXCEPTION(o)) {
        ip += location;
    }
    // TODO set the err
}

void call(int param_count) {
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
        interpret_block(func->co_consts, func->codes);
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
DEBUG_LOG(cerr << "returned " << result->type->type_name << endl;)
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
DEBUG_LOG(cerr << "returned " << instance->type->type_name << endl;)
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
        call(param_count);
        call_init();
    } else {
DEBUG_LOG(cerr << callable->type->type_name << endl;)
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
void loop(int location) {
    Object *it = TOP();
// next will consume this, should be optimized later on
    PUSH(it);
    newstr("next");
// TODO CHECK EXCEPTIONS!!!
    getmethod();
    call(1);
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
DEBUG_LOG(cerr << "swapping " <<  o1->type->type_name << " with " << o2->type->type_name << endl;)
    PUSH(o1);
    PUSH(o2);
}

std::vector<unsigned char> *read_func_code(std::vector<unsigned char> &codes) {
    std::stringstream ss;
    ss << "endfunction";
    std::vector<unsigned char> *funccode = new std::vector<unsigned char>;
    string code = ss.str();
    for (int i=0; i < code.size(); i++) {
        funccode->push_back(code.at(i));
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
    call(1);
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
DEBUG_LOG(cerr << "bp: " << bp << endl;)
    for (int i=0; i < gstack.size(); i++) {
        Object *o = gstack.at(i);
DEBUG_LOG(cerr << i << ": " << (o==NULL?"UNBOUND":o->type->type_name) << endl;)
    }
}


inline int next_arg(std::vector<unsigned char> &codes) {
    int val; 
    unsigned char higher = codes[ip+1];
    unsigned char lesser = codes[ip];
    val = (higher <<8) + lesser;
    if (higher & (1<<7)) {
        val -= (1<<16);
    }
ip+=2;
    return val;
}
// TODO assert!!
inline string get_const_str(std::vector<Object *> *co_consts, int i) {
DEBUG_LOG(cerr << "getconst" << i << endl;)
    String *str = static_cast<String *>(co_consts->at(i));
    return str->sval;
}

void interpret_block(std::vector<Object *> *co_consts, std::vector<unsigned char> &codes) {
    int block_bp = gstack.size() - 1;
    while (ip < codes.size()) {
        unsigned char command = codes[ip++];
        switch ((int)command) {
         case I_POP:
            if (TOP()->type != none_type && repl)
                print_func();
            if (!IS_EXCEPTION(TOP()))
                POP();
         break;
         case I_FUNCTION: {
// TODO check
            int startp = next_arg(codes); 
            int locals_count = next_arg(codes);
            int param_count = next_arg(codes);
            string name = "func";
            // TODO sanity check
DEBUG_LOG(cerr << "function code read " << startp << " param_count:" << param_count<<endl;)
// TODO
            newfunc(globals, co_consts, codes, ip + startp - 7, name, locals_count, param_count); // function is 7 bytes
DEBUG_LOG(cerr << "next: " << codes[ip+startp-7] << endl;)
         break;}
         case I_INT: {
            int ival = next_arg(codes);
DEBUG_LOG(cerr << "int:" << ival << endl;)
// TODO check  CONSTS
            co_consts->push_back(new Int(ival));
         break;}
         case I_STR: {
            string sval;
            for (; codes[ip] != '\0'; ip++) {
                char c = codes[ip];
                sval.push_back(c);
            }
            replace_all(sval, "\\n", "\n");
            ip++;
DEBUG_LOG(cerr << "str:" << sval << co_consts->size()<< endl;)
// TODO check CONSTS
            co_consts->push_back(new String(sval));
         break;}
         case I_DUP:
            gstack.push_back(gstack.back());
         break;
         case I_CALL: {
            int count = next_arg(codes);
DEBUG_LOG(cerr << "call " << count <<endl;)
            call(count);
         break;}
// TODO check
         case I_CLASS:
DEBUG_LOG(cerr << "class " << endl;)
            newclass_internal();
// TODO check
         break;
         case I_IMPORT: {
DEBUG_LOG(cerr << "import" << endl;)
            string module_name = get_const_str(co_consts, next_arg(codes));
            string var_name = get_const_str(co_consts, next_arg(codes));
            import(module_name, var_name);
         break;}
         case I_RETURN:
DEBUG_LOG(cerr << "return" << endl;)
             return;
         break;
// TODO check
         case I_PUSHLOCAL: {
            int lindex = next_arg(codes);
// TODO check
DEBUG_LOG(cerr << "pushlocal " << lindex << endl;)
            pushlocal(lindex);
         break;}
         case I_PUSHGLOBAL:{
            string name = get_const_str(co_consts, next_arg(codes));
// TODO check
DEBUG_LOG(cerr << "pushglobal " << name << endl;)
            pushglobal(name);
         break;}
         case I_SETLOCAL: {
            int lindex = next_arg(codes);
DEBUG_LOG(cerr << "setlocal " << lindex << endl;)
            setlocal(lindex);
         break;}
         case I_SETGLOBAL: {
            int index = next_arg(codes);
DEBUG_LOG(cerr << "setglobal " << index << endl;)
            string name = get_const_str(co_consts, index);
DEBUG_LOG(cerr << "setglobal " << name << endl;)
            setglobal(name);
         break;}
         case I_SWP:
DEBUG_LOG(cerr << "swp" << endl;)
            swp();
         break;
         case I_NOP:
DEBUG_LOG(cerr << "nop" << endl;)
         break;
         case I_GETFIELD:
DEBUG_LOG(cerr << "getfield" << endl;)
            getfield();
         break;
         case I_SETFIELD:
DEBUG_LOG(cerr << "setfield" << endl;)
            setfield();
         break;
         case I_SETITEM:
DEBUG_LOG(cerr << "setitem" << endl;)
            setitem();
         break;
         case I_GETMETHOD:
DEBUG_LOG(cerr << "getmethod" << endl;)
            getmethod();
         break;
         case I_JMP: {
            int location = next_arg(codes);
// TODO check
DEBUG_LOG(cerr << "jmp " << location << endl;)
            ip += location-3; // jmp is 3 bytes
         break;}
         case I_POP_TRAP_JMP: {
            traps->pop_back();
// TODO this instruction will be reconsidered
            int location = next_arg(codes);
// TODO check
DEBUG_LOG(cerr << "pop_trap_jmp " << location << endl;)
            ip += location-3; // pop_trap_jmp is 3 bytes
         break;}
         case I_LOOP:{
            int location = next_arg(codes);
DEBUG_LOG(cerr << "loop " << location << endl;)
            loop(location - 3); // loop is 3 bytes
         break;}
         case I_TRAP:{
            int location = next_arg(codes);
DEBUG_LOG(cerr << "trap " << location << endl;)
            trap(location - 3); // trap is 3 bytes
         break;}
         case I_ONERR:{
            int location = next_arg(codes);
DEBUG_LOG(cerr << "onerr " << location << endl;)
            onerr(location - 3); // onerr is 3 bytes
         break;}
         case I_JNT:{
            int location = next_arg(codes);
// TODO check
DEBUG_LOG(cerr << "jnt " << location << endl;)
            Object *o = POP();
            if (o == falseobject)
                ip += location - 3; // jnt is 3 bytes
         break;}
         case I_PUSHCONST: {
            int index = next_arg(codes);
DEBUG_LOG(cerr << "pushconst " << index << endl;)
            PUSH(co_consts->at(index));
            break;}
         case I_BUILD_LIST:{
            int count = next_arg(codes);
DEBUG_LOG(cerr << "build_list " << count << endl;)
            build_list(count);
         break;}
         case I_BUILD_DICT:{
            int count = next_arg(codes);
DEBUG_LOG(cerr << "build_dict " << count << endl;)
            build_dict(count);
         break;}
         default:
DEBUG_LOG(cerr << "command not defined" << command << endl;)
            throw std::exception();
        }
        if (gstack.size() > 0) {
            Object *exc = TOP();
            if (IS_EXCEPTION(exc)) {
                if (traps->size() > 0) {
                    int location = traps->back();
                    traps->pop_back();
                    ip = location;
                    POP(); // pop the exception
                    continue;
                }
                POP();
                gstack.resize(block_bp + 1);
                PUSH(exc);
                break;
            }
        }
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
DEBUG_LOG(cerr << "no stack to print" << endl;)
    }
}

void convert_codes(std::stringstream& fs, std::vector<unsigned char> &codes) {
    string code_str = fs.str();
    int i = 0;
    for (;i < code_str.size(); i++) {
        codes.push_back(code_str[i]);
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
 
void init_builtins(std::vector<unsigned char> *codes, int argc, char *argv[], char *env[]) {
    main_module = new Module(NULL, codes);
    globals = &main_module->fields;
    traps = new std::vector<int>();
    init_builtin_func();
    // TODO new instance functions should be implemented
    init_object();
    class_type = new Class("Class", NULL, -1);
    class_type->type = object_type;
    none_type = new Class("NoneType", NULL, 0);
    none = new Object();
    none->type = none_type;
    std::vector<Object *> *co_consts = new std::vector<Object *>();
    co_consts->push_back(none);
    main_module->co_consts = co_consts;
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
    (*globals)["None"] = none;
    builtins = globals;
    globals = new std::unordered_map<string, Object *>();
    std::vector<Object *> *sys_co_consts = new std::vector<Object *>();
    sys_co_consts->push_back(none);
    Module *sys_module = new Module(sys_co_consts, NULL);
    (*globals)["sys"] = sys_module;
    Int *o_argc = new Int(argc);
    sys_module->setfield("argc", o_argc);
    List *o_argv = new List();
    for (int i=0; i < argc; i++)
        o_argv->list->push_back(new String(argv[i]));
    sys_module->setfield("argv", o_argv);
    Dict *o_env = new Dict();
    while (*env != NULL) {
DEBUG_LOG(cerr << "parsing envvar: " << *env << endl;)
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
DEBUG_LOG(cerr << "compile command for " << module_name << ": " << compile_command << endl;)
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
