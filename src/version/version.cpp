#include "version.hpp"

#include <string>

#include <cstdlib>
#include "../shell/shell.h"
#include "../download/source.h"
#include <iostream>

namespace fs = std::filesystem;

#define PATH_MAKE_STRING(name) \
    const std::string& name##_string = name.string()

struct buildData {
    std::vector<std::string> tools;
    const char* version;
};

CommandResult buildToolchain(void* vdata)
{
    buildData* data = reinterpret_cast<buildData*>(vdata);

#ifdef _WIN32
    const char* cmd_base = "python -m ci.ci --no-test -v ";
#else
    const char* cmd_base = "python3 -m ci.ci --no-test -v ";
#endif

    std::string cmd = cmd_base;
    cmd += '\"';
    cmd += data->version;
    cmd += '\"';
    for (std::string& tool : data->tools) cmd += " " + tool;

    return invokeSystemCall(cmd.c_str());
}

void install_version(const char* version_str, const fs::path& source_dir, const fs::path& dest_dir, const std::vector<std::string>& tools, bool use_ansi)
{
    PATH_MAKE_STRING(source_dir);
    PATH_MAKE_STRING(dest_dir);

    sh_mkdir(source_dir_string.c_str());

    std::cout << "==> Downloading source of ";
    if (use_ansi) std::cout << "\033[36m";
    std::cout << version_str;
    if (use_ansi) std::cout << "\033[0m";
    std::cout << "..." << std::endl;
    char* archive = downloadSource(version_str, source_dir_string.c_str());
    if (!archive) {
        throw std::runtime_error(std::string("Couldn't download source of ") + version_str);
    }

    std::cout << "==> Unarchiving source of ";
    if (use_ansi) std::cout << "\033[36m";
    std::cout << version_str;
    if (use_ansi) std::cout << "\033[0m";
    std::cout << "..." << std::endl;
    char* unarchived = unpackSource(archive, source_dir_string.c_str(), version_str);
    if (!unarchived) {
        sh_remove(archive);
        std::free(archive);
        throw std::runtime_error(std::string("Couldn't unarchive source of ") + version_str);
    }

    const fs::path full_source = source_dir / unarchived;
    PATH_MAKE_STRING(full_source);

    std::free(archive);
    std::free(unarchived);

    buildData build_data;
    build_data.tools = tools;
    build_data.version = version_str;

    std::cout << "==> Building source of ";
    if (use_ansi) std::cout << "\033[36m";
    std::cout << version_str;
    if (use_ansi) std::cout << "\033[0m";
    std::cout << "..." << std::endl;
    CommandResult res = openDir(full_source_string.c_str(), buildToolchain, reinterpret_cast<void*>(&build_data));
    if (res.exit_code != 0) {
        sh_remove(full_source_string.c_str());
        throw std::runtime_error(std::string("Failed to build ") + version_str + ":\nSTDERR: " + res.stderr_str + "\nSTDOUT: " + res.stdout_str);
    }

    const fs::path full_dist = full_source / "dist";
    PATH_MAKE_STRING(full_dist);

    std::cout << "==> Copying 'dist/' of ";
    if (use_ansi) std::cout << "\033[36m";
    std::cout << version_str;
    if (use_ansi) std::cout << "\033[0m";
    std::cout << "..." << std::endl;
    res = copy(full_dist_string.c_str(), dest_dir_string.c_str());
    if (res.exit_code != 0) {
        sh_remove(full_source_string.c_str());
        throw std::runtime_error(std::string("Couldn't copy \"dist\" of ") + version_str + ":\nSTDERR: " + res.stderr_str + "\nSTDOUT: " + res.stdout_str);
    }

    sh_remove(full_source_string.c_str());
}

void uninstall_version(const fs::path& dest_dir, const std::vector<std::string>& tools)
{
    const fs::path bin_dir = dest_dir / "bin";
    const fs::path tpl_dir = dest_dir / "THIRD_PARTY_LICENSES";

    for (const std::string& tool : tools) {
#ifdef _WIN32
        const fs::path executable = bin_dir / (tool + ".exe");
#else
        const fs::path executable = bin_dir / tool;
#endif
        const fs::path tpl = tpl_dir / (tool + ".txt");

        sh_remove(executable.string().c_str());
        if (fs::exists(tpl)) sh_remove(tpl.string().c_str());
    }
}
