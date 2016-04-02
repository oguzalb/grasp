#include "vm.h"
#include "string.h"

extern string main_path;
extern string stack_machine_path;

// TODO fix in repl also
string find_path(string file_path) {
    string path;
    if (file_path[0] == '/') {
        int index = file_path.rfind("/");
        path = file_path.substr(0, index+1).c_str();
    } else if (file_path.find("/") != -1) {
        // TODO will be reduced later, keeping to remember
        int index = file_path.rfind("/");
        path = file_path.substr(0, index+1).c_str();
    } else {
        path = ".";
    }
    return path;
}

int main (int argc, char *argv[], char *env[]) {
    if (argc != 2) {
        cerr << "Needs file as an argument" << endl;
        return 1;
    }
    stack_machine_path = find_path(argv[0]);
    string sourcefilename = argv[1];
    main_path = find_path(sourcefilename);
    if (!ends_with(sourcefilename, ".graspo")) {
        cerr << "Needs a file with extension .graspo as an argument" << endl;
        return 1;
    }
    std::vector<std::string> *codes = new std::vector<std::string>();
    init_builtins(codes, argc, argv, env);
    std::stringstream *ss = read_codes(sourcefilename);
    if (ss != NULL) {
        convert_codes(*ss, *codes);
        delete ss;
        interpret_block(*codes);
    }
    print_stack_trace();

    return 0;
}
