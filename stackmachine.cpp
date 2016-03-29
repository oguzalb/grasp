#include "vm.h"
#include "string.h"

extern string main_path;

int main (int argc, char *argv[], char *env[]) {
    if (argc != 2) {
        cerr << "Needs file as an argument" << endl;
        return 1;
    }
    string sourcefilename = argv[1];
    if (sourcefilename[0] == '/') {
        int index = sourcefilename.rfind("/");
        main_path = sourcefilename.substr(0, index+1).c_str();
    } else if (sourcefilename.find("/") != -1) {
        // TODO will be reduced later, keeping to remember
        int index = sourcefilename.rfind("/");
        main_path = sourcefilename.substr(0, index+1).c_str();
    } else {
        main_path = ".";
    }
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
