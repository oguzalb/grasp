#include "vm.h"
#include "assert.h"
enum {INT_TYPE, FUNC_TYPE, NONE_TYPE, STR_TYPE, BOOL_TYPE, LIST_TYPE, LISTITERATOR_TYPE};

void Object::setfield(string name, Object* object) {
    this->fields.insert({name, object});
}

Object *Object::getfield(string name) {
// TODO exceptions
    return this->fields.at(name);
}

Bool *trueobject;
Bool *falseobject;

Bool *newbool_internal(int bval) {
    Bool *o = new Bool;
    o->type = BOOL_TYPE;
    o->bval = bval;
    cout << "newbool: " << bval << endl;
    return o;
}

void newint(int ival) {
    Object *o = new Object;
    o->type = INT_TYPE;
    o->ival = ival;
    cout << "newint: " << ival << endl;
    gstack.push_back(o);
}

void newstr(string sval) {
    Object *o = new Object;
    o->type = STR_TYPE;
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
    o->type = FUNC_TYPE;
    o->codep = startp;
    gstack.push_back(o);
}

void interpret_block(std::vector<std::string>& codes);

void call(std::vector<std::string>& codes, int param_count) {
    // TODO stuff about param_count
    bp = gstack.size() - param_count;
    Function *func = (Function *)gstack.at(bp - 1);
    // TODO exc
    assert(func->type == FUNC_TYPE);
    int cur_ip = ip;
    ip = func->codep;
    interpret_block(codes);
    Object *result = gstack.back();
    gstack.pop_back();
    gstack.resize(gstack.size() - param_count);
    func = (Function *)gstack.back();
    assert(func->type == FUNC_TYPE);
    gstack.pop_back();
    gstack.push_back(result);
    ip = cur_ip;
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
            if (val->type == INT_TYPE)
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
        } else if (command == "jmp") {
            string label;
            ss >> label;
            int location = labels.at(label);
// TODO check
            cout << "jmp " << location << endl;
            ip = location-1; // will increase at the end of loop
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

int main () {
    std::fstream fs;
    trueobject = newbool_internal(TRUE);
    falseobject = newbool_internal(FALSE);
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
