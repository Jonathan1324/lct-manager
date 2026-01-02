extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
}

#include <filesystem>
#include "shell/shell.h"
#include "download/source.h"

namespace fs = std::filesystem;

int buildToolchain(void* null)
{
    (null);
#ifdef _WIN32
    const char* cmd = "python -m ci.ci --no-test";
#else
    const char* cmd = "python3 -m ci.ci --no-test";
#endif
    return system(cmd);
}

typedef unsigned char Command;
#define COMMAND_INSTALL ((Command)1)

#define PATH_MAKE_STRING(name) \
    const std::string& name##_string = name.string()

int main(int argc, const char* argv[])
{
    const char* version_str = "v0.1.0-alpha.5";
    
    const fs::path main_dir = "test";

    const fs::path source_dir = main_dir / "archives";
    PATH_MAKE_STRING(source_dir);
    const fs::path dest_dir = main_dir / "b";
    PATH_MAKE_STRING(dest_dir);

    Command command = COMMAND_INSTALL;

    switch (command)
    {
        case COMMAND_INSTALL: {
            sh_mkdir(source_dir_string.c_str());
            char* name = downloadSource(version_str, source_dir_string.c_str());
            if (!name) {
                fputs("Couldn't download source", stderr);
                return 1;
            }
            const fs::path full_source = source_dir / name;
            PATH_MAKE_STRING(full_source);

            free(name);

            openDir(full_source_string.c_str(), buildToolchain, NULL);

            const fs::path full_dest = dest_dir / version_str;
            PATH_MAKE_STRING(full_dest);
            const fs::path full_dist = full_source / "dist" / ".";
            PATH_MAKE_STRING(full_dist);

            copy(full_dist_string.c_str(), full_dest_string.c_str());

            sh_remove(full_source_string.c_str());

            break;
        }
        
        default:
            break;
    }

    return 0;
}
