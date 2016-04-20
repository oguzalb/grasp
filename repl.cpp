#include "vm.h"
#include "string.h"
#include <unistd.h>
#include <sys/param.h>

extern std::vector<Object *> gstack;
extern Class *exception_type;
extern string main_path;
extern Object *none;
extern bool repl;
extern int ip;

std::stringstream *compile(string code) {
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
   int status = pclose(fpipe);
   if (WEXITSTATUS(status) != 0) {
       return NULL;
   }

   std::fstream fs;
   std::stringstream *ss = new std::stringstream;
// TODO handling stuff
   fs.open("repl.graspo", std::fstream::in);
   copy(istreambuf_iterator<char>(fs),
     istreambuf_iterator<char>(),
     ostreambuf_iterator<char>(*ss));
   fs.close();
// TODO might be unsuccessful
   return ss;
}

std::string get_working_path()
{
   char temp[MAXPATHLEN];
   getcwd(temp, MAXPATHLEN);
   return std::string(temp);
}

int main (int argc, char *argv[], char *env[]) {
    repl = true;
    main_path = get_working_path();
    std::vector<unsigned char> *codes = new std::vector<unsigned char>();
    init_builtins(codes, argc, argv, env);
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
        std::stringstream *ss = compile(code);
        if (ss == NULL) {
            // grasp.py gives the error already
            continue;
        }
        codes = new std::vector<unsigned char>;
        convert_codes(*ss, *codes);
        delete ss;
        Module *repl_block = new Module(codes);
        ip = 0;
        interpret_block(repl_block);
        if (gstack.size() > 0) {
            Object *exc = TOP();
            assert(exc->isinstance(exception_type));
            print_func();
            POP();
        }
    };
    return 0;
}
