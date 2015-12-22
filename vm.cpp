#include "vm.h"
#include "assert.h"

Bool *trueobject;
Bool *falseobject;
Object *int_type;
Object *func_type;
Object *builtinfunc_type;
Object *none_type;
Object *none;
Object *bool_type;
Object *str_type;
Object *list_type;
Object *listiterator_type;
Object *exception_type;


void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
}

Object *Object::getfield(string name) {
// TODO exceptions
// TODO parent chain   
    Object *field;
    try {
        field = this->fields.at(name);
    } catch (const std::out_of_range& oor) {
        assert(this->type);
        try {
            field = this->type->fields.at(name);
        } catch (const std::out_of_range& oor) {
            throw exception();
        }
    }
    cout << "getfield type " << field->type << endl;
    return field;
}

ListIterator::ListIterator(std::vector<Object *> *list) {
    this->it = new std::vector<Object *>::iterator(list->begin());
cout << "new iterator points to" << *(*this->it) << endl;
    this->end = new std::vector<Object *>::iterator(list->end());
}

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
    Object *m = new Object();
    m->sval = message;
    e->setfield("message", m);
    error = e;
}

inline Object *newint_internal(int ival) {
    Object *o = new Object;
    o->type = int_type;
    o->ival = ival;
    cout << "newint: " << ival << endl;
    return o;
}

void newint(int ival) {
    Object *o = newint_internal(ival);
    gstack.push_back(o);
}

void newstr(string sval) {
    Object *o = new Object;
    o->type = str_type;
    o->sval = sval;
    cout << "newstr: " << sval << endl;
    gstack.push_back(o);
}

void newnone() {
    Object *o = new Object;
    // TODO won't stay like this
    o->ival = -100;
    gstack.push_back(o);
}

void newfunc(int startp) {
    Function *o = new Function;
    o->type = func_type;
    o->codep = startp;
    gstack.push_back(o);
}

void setfield() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    gstack.pop_back();
    Object *o3 = gstack.back();
    gstack.pop_back();
    o3->setfield(o2->sval, o1);
    cout << "field set: " << o2->sval << endl;
}

void getfield() {
    Object *o1 = gstack.back();
cout << "object type" << o1->type << endl;
    gstack.pop_back();
    Object *o2 = gstack.back();
cout << "object type" << o2->type << endl;
    gstack.pop_back();
    Object *field =o2->getfield(o1->sval);
    gstack.push_back(field);
    cout << "field pushed" << endl;
}

void getmethod() {
    Object *o1 = gstack.back();
cout << "object type" << o1->type << endl;
    gstack.pop_back();
    Object *o2 = gstack.back();
cout << "object type" << o2->type << endl;
    Object *field = o2->getfield(o1->sval);
    assert(field->type == builtinfunc_type || field->type == func_type);
    gstack.push_back(field);
    cout << "field pushed type " << field->type << endl;
}

void setglobal(string name) {
    globals[name] = gstack.back();
    gstack.pop_back();
}

void setlocal(int ival) {
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

void pushlocal(int ival) {
    if (ival >= gstack.size() - bp) {
        cerr << "OUPSSS, size bigger than locals" << endl;
        exit(1);
    }
    cout << "pushed type " << gstack[bp + ival]->type << endl;
    gstack.push_back(gstack[bp + ival]);
}


void interpret_block(std::vector<std::string>& codes);

void call(std::vector<std::string>& codes, int param_count) {
    // TODO stuff about param_count
    bp = gstack.size() - param_count;
    Object *callable = gstack.at(bp - 1);
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
        assert(func->type == func_type);
        gstack.pop_back();
        gstack.push_back(result);
        ip = cur_ip;
    } else if (callable->type == builtinfunc_type) {
        BuiltinFunction *func = static_cast<BuiltinFunction *>(callable);
        func->function();
        Object *result = gstack.back();
        gstack.pop_back();
// builtin funcs consume the parameters
        assert(gstack.back()->type == builtinfunc_type);
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
        ip = location;
    }
}

void add() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    gstack.pop_back();
    // TODO exc
    newint(o1->ival + o2->ival);
}
 
void sub() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    gstack.pop_back();
    // TODO exc
    newint(o2->ival - o1->ival);
}
 
void mul() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
    gstack.pop_back();
    // TODO exc
    newint(o1->ival * o2->ival);
}
 
void div() {
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
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
    Object *o1 = gstack.back();
    gstack.pop_back();
    Object *o2 = gstack.back();
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
    int ret = FALSE;
    while (ip < codes.size()) {
        string command;
        string param;
        std::stringstream ss(codes[ip]);
        ss >> command;
        if (command == "pop") {
            Object *val = gstack.back();
            if (val->type == int_type)
                cout << "popped " << val->ival << endl;
            else
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
            newfunc(startp);
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
        } else if (command == "call") {
            int count;
            ss >> count;
            cout << "call " << count <<endl;
            call(codes, count);
            if (error != NULL)
                throw exception();
// TODO check
        } else if (command == "return") {
            cout << "return " << endl;
            ret = TRUE;
            break;
// TODO check
        } else if (command == "pushlocal") {
            int lindex;
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
    if (ret == FALSE)
        newnone();
}

void read_codes(std::stringstream& fs, std::vector<std::string>& codes) {
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

Object *new_type() {
    Object *type = new Object();
    type->type = NULL;
    return type;
}

void range_func() {
    Object *max = gstack.back();
    gstack.pop_back();
    List *list = new List();
    list->type = list_type;
    list->list = new std::vector<Object *>();
    for (int i=0; i<max->ival; i++)
        list->list->push_back(newint_internal(i));
    gstack.push_back(list);
}

void print_func() {
    Object *o = gstack.back();
    gstack.pop_back();
    cout << o->ival << endl;
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
    Object *bool_type = new Object();
    trueobject = newbool_internal(TRUE);
    falseobject = newbool_internal(FALSE);
    exception_type = new_type();
    int_type = new_type();
    func_type = new_type();
    none_type = new_type();
    str_type = new_type();
    list_type = new_type();
    builtinfunc_type = new_type();
    BuiltinFunction *range = newbuiltinfunc_internal(range_func);
    globals["range"] = range;
    BuiltinFunction *iter_func = newbuiltinfunc_internal(list_iter);
    list_type->setfield("iter", iter_func);
    listiterator_type = new_type();
    BuiltinFunction *next_func = newbuiltinfunc_internal(listiterator_next);
    listiterator_type->setfield("next", next_func);
    BuiltinFunction *print = newbuiltinfunc_internal(print_func);
    globals["print"] = print;
    none_type = new_type();
    none = new Object();
    none->type = none_type;
}

int main () {
    init_builtins();
    std::fstream fs;
    fs.open("test.graspo", std::fstream::in);
    std::stringstream ss;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    std::vector<std::string> codes;
    read_codes(ss, codes);
    ip = 0;
    interpret_block(codes);
    return 0;
}
