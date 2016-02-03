#include "vm.h"
#include "string.h"

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
    std::fstream fs;
    fs.open(sourcefilename, std::fstream::in);
    std::stringstream ss;
    std::vector<std::string> codes;
    copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
    read_codes(ss, codes);
    interpret_block(codes);
    return 0;
}
