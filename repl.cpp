#include "vm.h"
#include "string.h"
extern std::vector<Object *> gstack;
extern Class *exception_type;

std::stringstream compile(string code) {
   FILE *fpipe;
   char *command="python grasp.py";

   if (!(fpipe = (FILE*)popen(command,"w")) ) {
      perror("Problems with pipe");
      exit(1);
   }
   fwrite(code.c_str(), 1, code.size(), fpipe);
   fflush(fpipe);
   char buf[1024];
   std::string  cur_string = "";
   while (fgets(buf, sizeof (buf), fpipe) != NULL) {
       cur_string += buf;
   }
   pclose(fpipe);

   std::fstream fs;
   std::stringstream ss;
// TODO handling stuff
   fs.open("repl.graspo", std::fstream::in);
   copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(ss));
   fs.close();
// TODO might be unsuccessful
   return ss;
}

int main (int argc, char *argv[]) {
    std::vector<std::string> codes;
    init_builtins(&codes);
    string code;
    while (1) {
        string line;
        cout << ">>";
        code = "";
        while (!ends_with(code, "\n\n")) {
            getline(cin, line);
            if (line == "quit")
                return 0;
            code += line + "\n";
            cout << "..";
        }
        std::stringstream ss = compile(code);
        convert_codes(ss, codes);
        dump_codes(codes);
        interpret_block(codes);
        if (gstack.size() > 0) {
            Object *exc = POP();
            assert(exc->isinstance(exception_type));
        }
    };
    return 0;
}
