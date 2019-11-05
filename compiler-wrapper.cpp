#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <vector>


using namespace std;

static char** cc_params;              /* Parameters passed to the real CC  */
static size_t cc_par_cnt = 1;         /* Param count, including argv0      */
bool c_mode = true;                   /* Is is c or c++ code               */
vector<string> params;                /* Params in a vectorl               */


bool check_variable(const string line, const string env) {
    if (line.find(env+"=") != string::npos) {
        string wrap_env = "WRAP_" + env;
        setenv(wrap_env.c_str(), line.c_str() + env.size() + 1, false);
        return true;
    }
    return false;
}

bool check_variables(const string line) {
    bool set = false;
    set |= check_variable(line, "CC");
    set |= check_variable(line, "CXX");
    set |= check_variable(line, "CFLAGS");
    set |= check_variable(line, "CXXFLAGS");
    set |= check_variable(line, "LDFLAGS");
    return set;
}

void print_variable(const char* var) {
    printf("%s=%s\n", var, getenv(var));
}

void print_variables() {
    print_variable("WRAP_CC");
    print_variable("WRAP_CXX");
    print_variable("WRAP_CFLAGS");
    print_variable("WRAP_CXXFLAGS");
    print_variable("WRAP_LDFLAGS");
}

void add_flag_to_params(vector<string> &params, const char*var) {
    string flag(var);
    size_t current, previous = 0;
    current = flag.find_first_of(" ");
    while (current != string::npos) {
        params.push_back(flag.substr(previous, current - previous));
        previous = current + 1;
        current = flag.find_first_of(" ", previous);
    }
    params.push_back(flag.substr(previous, current - previous));
}

void edit_params(int argc, char* argv[]) {
    bool maybe_linking = true;
    

    while(--argc) {
        string cur = argv[argc];

        if (cur == "-c" || cur == "-S" || cur == "-E")
            maybe_linking = false;

        params.push_back(cur);
    }

    add_flag_to_params(params, getenv(c_mode? "WRAP_CFLAGS": "WRAP_CXXFLAGS"));
    
    if (maybe_linking) {
        add_flag_to_params(params, getenv("WRAP_LDFLAGS"));
    }

    cc_params = new char* [params.size() + 2];

    for (int i = 0; i < params.size(); i++) {
        cc_params[i+1] = (char*) params[i].c_str();
    }
    
    cc_params[0] = getenv(c_mode? "WRAP_CC": "WRAP_CXX");
    cc_params[params.size() + 1] = nullptr;

    /*
    for (int i = 0; i < params.size() +1 ; i++) {
        printf("%s ", cc_params[i]);
    }
    printf("\n");
    */
}

int main(int argc, char* argv[]) {

    string bin = string(argv[0]);
    string dir = bin.substr(0, bin.find_last_of('/'));
    string config = dir + "/config";

    if (argc < 2) {
        printf("\n"
               "This is a wrapper application that force some flags\n"
               "A common use pattern would be one of the following:\n\n"
               
               "  CC=%s/compiler-wrapper ./configure\n"
               "  CXX=%s/compiler-wrapper++ ./configure\n\n"

               "You can either edit the config file \"%s\", or use WRAP_*\n"
               "environment variable to set flags during compiling.\n"
               "They include WRAP_CC, WRAP_CXX, WRAP_CFLAGS, WRAP_CXXFLAGS\n"
               "and WRAP_LDFLAGS\n\n",
               dir.c_str(), dir.c_str(), config.c_str()
               );
        exit(1);
    }

    if (bin.find_last_of("++") == bin.size()-1) 
        c_mode = false;

    cout << "Binary: " << bin << endl;
    cout << "Directory: " << dir << endl;

    ifstream config_file(config, ios::in);
    string line;
    while(getline(config_file, line)) {
        if (!check_variables(line)) {break;}
    }
    config_file.close();

    //print_variables();

    edit_params(argc, argv);

    execvp(cc_params[0], (char**)cc_params);

    cerr << "Failed to execute " << string(getenv(c_mode? "WRAP_CC": "WRAP_CXX")) << endl;
    return 0;

}