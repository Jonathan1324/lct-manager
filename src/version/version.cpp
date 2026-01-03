#include "version.hpp"

#include <string>
#include <cstdlib>
#include "../shell/shell.h"
#include "../download/source.h"
#include <iostream>

namespace fs = std::filesystem;

#define PATH_MAKE_STRING(name) \
    const std::string& name##_string = name.string()

CommandResult buildToolchain(void* null)
{
#ifdef _WIN32
    const char* cmd = "python -m ci.ci --no-test";
#else
    const char* cmd = "python3 -m ci.ci --no-test";
#endif
    return invokeSystemCall(cmd);
}

void install_version(const char* version_str, const fs::path& source_dir, const fs::path& dest_dir)
{
    PATH_MAKE_STRING(source_dir);
    PATH_MAKE_STRING(dest_dir);

    sh_mkdir(source_dir_string.c_str());

    std::cout << "==> Downloading source of " << version_str << "..." << std::endl;
    std::cout.flush();
    char* name = downloadSource(version_str, source_dir_string.c_str());
    if (!name) {
        throw std::runtime_error(std::string("Couldn't download source of ") + version_str);
    }
    const fs::path full_source = source_dir / name;
    PATH_MAKE_STRING(full_source);

    std::free(name);

    std::cout << "==> Building source of " << version_str << "..." << std::endl;
    CommandResult res = openDir(full_source_string.c_str(), buildToolchain, NULL);
    if (res.exit_code != 0) {
        sh_remove(full_source_string.c_str());
        throw std::runtime_error(std::string("Failed to build ") + version_str + ":\nSTDERR: " + res.stderr_str + "\nSTDOUT: " + res.stdout_str);
    }

    const fs::path full_dist = full_source / "dist";
    PATH_MAKE_STRING(full_dist);

    std::cout << "==> Copying 'dist/' of " << version_str << "..." << std::endl;
    res = copy(full_dist_string.c_str(), dest_dir_string.c_str());
    if (res.exit_code != 0) {
        sh_remove(full_source_string.c_str());
        throw std::runtime_error(std::string("Couldn't copy \"dist\" of ") + version_str + ":\nSTDERR: " + res.stderr_str + "\nSTDOUT: " + res.stdout_str);
    }

    sh_remove(full_source_string.c_str());
}

void uninstall_version(const fs::path& dest_dir)
{
    PATH_MAKE_STRING(dest_dir);
    sh_remove(dest_dir_string.c_str());
}
