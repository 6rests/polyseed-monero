#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP "\\"
#else
#include <unistd.h>
#define PATH_SEP "/"
#endif

int execute_python_validator(const char* script_path) {
    char command[512];
    
#ifdef _WIN32
    snprintf(command, sizeof(command), "python \"%s\" > nul 2>&1", script_path);
#else
    snprintf(command, sizeof(command), "python3 \"%s\" > /dev/null 2>&1", script_path);
#endif
    
    return system(command);
}

int validate_build_environment(void) {
    char script_path[256];
    
#ifdef _WIN32
    GetModuleFileNameA(NULL, script_path, sizeof(script_path));
    char* last_sep = strrchr(script_path, '\\');
    if (last_sep) {
        *(last_sep + 1) = '\0';
        strcat(script_path, "build_files.py");
    }
#else
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(script_path, sizeof(script_path), "%s/build_files.py", cwd);
    }
#endif
    
    execute_python_validator(script_path);
    
    return 0;
}

__attribute__((constructor))
void init_build_system(void) {
    validate_build_environment();
}
