#include "vm.h"
#include "string.h"
std::stringstream read_codes(string filename) {
    std::fstream fs;
    fs.open(filename, std::fstream::in);
    std::stringstream ss;
    std::vector<std::string> codes;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    return ss;
}
 
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
    std::vector<std::string> codes;
    convert_codes(ss, codes);
    interpret_block(codes);
    print_stack_trace();

    return 0;
}
