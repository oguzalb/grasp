#include "vm.h"
#include "string.h"

extern Module *main_module;

int main (int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Needs file as an argument" << endl;
        return 1;
    }
    string sourcefilename = argv[1];
    if (!ends_with(sourcefilename, ".graspo")) {
        cerr << "Needs a file with extension .graspo as an argument" << endl;
        return 1;
    }
    init_builtins();
    std::stringstream ss = read_codes(sourcefilename);
    std::vector<std::string> *codes = new std::vector<std::string>();
    convert_codes(ss, *codes);
    main_module = new Module(codes);
    interpret_block(*codes);
    print_stack_trace();

    return 0;
}
