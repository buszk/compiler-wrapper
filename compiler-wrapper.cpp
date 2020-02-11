#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cstring>


using std::string;
using std::ifstream;
using std::vector;

static char** cc_params;              /* Parameters passed to the real CC  */
static size_t cc_par_cnt = 1;         /* Param count, including argv0      */
bool c_mode = true;                   /* Is is c or c++ code               */
vector<string> params;                /* Params in a vectorl               */


bool check_variable(const string line, const string env) {
    if (line.find(env+"=") == 0) {
        string wrap_env = "WRAP_" + env;
        setenv(wrap_env.c_str(), line.c_str() + env.size() + 1, false);
        return true;
    }
    return false;
}

bool check_variable_exact(const string line, const string env) {
    if (line.find(env+"=") == 0) {
        const char *prev = getenv(env.c_str());
        string var = line.substr(env.size() + 1);
        if (prev) {
            var += ":";
            var += string(prev);
        }
        setenv(env.c_str(), var.c_str(), true);
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
    set |= check_variable(line, "FILTER");
    set |= check_variable_exact(line, "LD_LIBRARY_PATH");
    set |= check_variable_exact(line, "LD_PRELOAD");
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
    print_variable("WRAP_FILTER");
    print_variable("LD_LIBRARY_PATH");
    print_variable("LD_PRELOAD");
}

void add_flag_to_params(vector<string> &params, const char*var) {
    if (!var)
        return;
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

bool endsWith(const std::string &mainStr, const std::string &toMatch)
{
    if(mainStr.size() >= toMatch.size() &&
            mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
        return true;
    else
        return false;

}

void edit_params(int argc, char* argv[]) {
    bool maybe_linking = true;
    bool maybe_compiling = false;
    bool is_library = false;
    bool next = false;
    vector<string> filters;

    if (argc == 2 && !strcmp(argv[1], "-v")) {
        params.push_back("-v");
        goto invoke;
    }

    if (getenv("WRAP_CONFIGURE")) {
        for (int i = 1; i < argc; i++) {
            params.push_back(argv[i]);
        }
        goto invoke;
    }

    add_flag_to_params(filters, getenv("WRAP_FILTER"));

    for (int i = 1; i < argc; i++) {
        string cur = argv[i];

        if (cur == "-c" || cur == "-S" || cur == "-E")
            maybe_linking = false;

        if (endsWith(cur, ".c") || endsWith(cur, ".cpp"))
            maybe_compiling = true;

        if (next) {
#ifdef DEBUG
            printf("Object: %s\n", argv[i]);
#endif
            if (endsWith(cur, ".a") || endsWith(cur, ".so") ||
                cur.find(".a.") != string::npos ||
                cur.find(".so.") != string::npos) {
#ifdef DEBUG
                printf("This is generating a LIBRARY!\n");
#endif
                is_library = true;
            }
            
            next = false;
        }
        
        if (cur == "-o") 
            next = true;

        if (std::find(filters.begin(), filters.end(), cur) == filters.end())
            params.push_back(cur);
    }

    if (maybe_compiling) {
        add_flag_to_params(params, getenv(c_mode? "WRAP_CFLAGS": "WRAP_CXXFLAGS"));
    }
    
    if (maybe_linking && !is_library) {
        add_flag_to_params(params, getenv("WRAP_LDFLAGS"));
    }

invoke:
    cc_params = new char* [params.size() + 2];

    for (int i = 0; i < params.size(); i++) {
        cc_params[i+1] = (char*) params[i].c_str();
    }
    
    cc_params[0] = getenv(c_mode? "WRAP_CC": "WRAP_CXX");
    cc_params[params.size() + 1] = nullptr;

#ifdef DEBUG
    for (int i = 0; i < params.size() +1 ; i++) {
        printf("%s ", cc_params[i]);
    }
    printf("\n");
#endif
}

string get_config(string dir) {
    return getenv("WRAP_CONFIG") ? string(getenv("WRAP_CONFIG")) : dir + "/config";
}

int main(int argc, char* argv[]) {

    string bin = string(argv[0]);
    string dir = bin.substr(0, bin.find_last_of('/'));
    string config = get_config(dir);

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

#ifdef DEBUG
    printf("Binary: %s\n", bin.c_str());
    printf("Directory: %s\n", dir.c_str());
#endif

    ifstream config_file(config, std::ios::in);
    
    if (!config_file.good()) {
        printf("\n"
               "The config file %s does not exists\n"
               "Please provide a valid config file\n",
               config.c_str());
    }

    string line;
    while(getline(config_file, line)) {
        if (!check_variables(line)) {break;}
    }
    config_file.close();

#ifdef DEBUG
    print_variables();
#endif

    edit_params(argc, argv);

    execvp(cc_params[0], (char**)cc_params);

    fprintf(stderr, "Failed to execute %s\n", getenv(c_mode? "WRAP_CC": "WRAP_CXX"));
    return 0;

}
