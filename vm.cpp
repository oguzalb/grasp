#include "vm.h"
#include "assert.h"
enum {INT_TYPE, FUNC_TYPE, NONE_TYPE};

void newint(int ival) {
    Object *o = new Object;
    o->type = INT_TYPE;
    o->ival = ival;
    gstack.push_back(o);
}

void newnone() {
    Object *o = new Object;
    // TODO won't stay like this
    o->ival = -100;
    gstack.push_back(o);
}

void newfunc(std::stringstream *code) {
    Object *o = new Object;
    o->type = FUNC_TYPE;
    o->code = code;
    gstack.push_back(o);
}

void interpret_block(std::stringstream& fs);

void call(int param_count) {
    bp = gstack.size() - param_count;
    Object *func = gstack.at(bp - 1);
    // TODO exc
    assert(func->type == FUNC_TYPE);
    interpret_block(*(func->code));
    Object *result = gstack.back();
    gstack.pop_back();
    gstack.resize(gstack.size() - param_count);
    func = gstack.back();
    assert(func->type == FUNC_TYPE);
    gstack.pop_back();
    gstack.push_back(result);
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
    newint(o1->ival - o2->ival);
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

void setglobal(string name) {
    globals[name] = gstack.back();
    gstack.pop_back();
}

void setlocal(int ival) {
    if (ival >= gstack.size() - bp) {
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

std::stringstream *read_func_code(std::stringstream& fs, string name) {
    std::stringstream ss;
    ss << "endfunction";
    std::string endcommand = ss.str();
    std::stringstream *funccode = new std::stringstream;
    string line;
    while (getline(fs, line) && line != endcommand) {
        *funccode << line << endl;
    }
    return funccode;
}

void interpret_block(std::stringstream& fs) {
    string line;
    int ret = FALSE;
    while (getline(fs, line)) {
        string command;
        string param;
        std::stringstream ss(line);
        ss >> command;
        if (command == "pop") {
            Object *val = gstack.back();
            if (val->type == INT_TYPE)
                cout << "popped " << val->ival << endl;
            else
                cout << "popped func" << endl;
            gstack.pop_back();
        } else if (command == "function") {
            string name;
            ss >> name;
            cout << "function " << name << endl;
// TODO check
            std::stringstream *code = read_func_code(fs, name);
            cout << "function code read " << name << endl;
            newfunc(code);
        } else if (command == "int") {
            int ival;
            ss >> ival;
// TODO check
            cout << "newint " << ival << endl;
            newint(ival);
        } else if (command == "call") {
            int base;
            ss >> base;
            cout << "call " << base <<endl;
            call(base);
// TODO check
        } else if (command == "return") {
            cout << "return " << endl;
            ret = TRUE;
            break;
// TODO check
        } else if (command == "pushlocal") {
            int lindex;
            ss >> lindex;
            cout << "pushlocal " << lindex << endl;
// TODO check
            pushlocal(lindex);
        } else if (command == "pushglobal") {
            string name;
            ss >> name;
// TODO check
            pushglobal(name);
            cout << "pushglobal " << name << endl;
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
        } else {
            cerr << "command not defined" << command << endl;
        }
    }
    if (ret == FALSE)
        newnone();
}

int main () {
    std::fstream fs;
    fs.open("test.graspo", std::fstream::in);
    std::stringstream ss;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    interpret_block(ss);
    return 0;
}
