#include <iostream>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "version/version.hpp"
#include "home/home.hpp"
#include "data/state.hpp"
#include "terminal/terminal.h"
#include "shell/shell.h"
#include "info.hpp"

namespace fs = std::filesystem;

typedef unsigned char Command;
#define COMMAND_NONE        ((Command)0)
#define COMMAND_HELP        ((Command)1)
#define COMMAND_VERSION     ((Command)2)
#define COMMAND_INSTALL     ((Command)3)
#define COMMAND_UNINSTALL   ((Command)4)
#define COMMAND_REINSTALL   ((Command)5)
#define COMMAND_UPDATE      ((Command)6)
#define COMMAND_LIST        ((Command)7)
#define COMMAND_PATH        ((Command)8)
#define COMMAND_REMOVE      ((Command)9)
#define COMMAND_INFO        ((Command)10)

#ifdef DEBUG_BUILD
#define DO_LOCAL_TEST 1
#else
#define DO_LOCAL_TEST 0
#endif

#define VERSION "v0.1.0-alpha.3"

const char* first_version = "v0.1.0-alpha.5-jan2026.4";
const char* latest_version = "v0.1.0-alpha.5-feb2026.1";

static std::unordered_map<std::string, int> versions = {
    {"v0.1.0-alpha.5-jan2026.4", 0},
    {"v0.1.0-alpha.5-jan2026.5", 1},
    {"v0.1.0-alpha.5-feb2026.1", 2}
};

static std::unordered_map<std::string, std::vector<std::string>> valid_tools_deps = {
    {"lhoho", {}},
    {"ljoke", {}},
    {"lbf",   {}},
    {"lfs",   {}},
    {"lbt",   {}},
    {"lnk",   {}},
    {"lasm",  {"lasmp"}},
    {"lasmp", {}}
};

static std::unordered_map<std::string, std::vector<std::string> > bundles = {
    {"all", {"lasmp", "lasm", "lnk", "lbt", "lfs", "lbf", "ljoke", "lhoho"}},

    {"toolchain", {"lasmp", "lasm", "lnk"}},

    {"fun", {"lbf", "ljoke", "lhoho"}}
};

void printVersion(bool use_ansi)
{
    std::cout << "LCT Manager " << VERSION << std::endl;
    std::cout << "Supports LCT " << first_version << " through " << latest_version << std::endl;
    std::cout << "Compiled on " << __DATE__ << std::endl;
    std::cout << "License: BSD 3-Clause" << std::endl;

    std::cout.flush();
}

void printHelp(const char* name, std::ostream& out, bool use_ansi)
{
    out << "Usage: " << name << " <command> <args>" << std::endl;

    out << "> " << name << " install <tools>" << std::endl;
    out << "> " << name << " uninstall <tools>" << std::endl;
    out << "> " << name << " reinstall <tools>" << std::endl;
    out << "> " << name << " update <tools>" << std::endl;
    out << "> " << name << " list" << std::endl;
    out << "> " << name << " path" << std::endl;
    out << "> " << name << " remove" << std::endl;
    out << "> " << name << " info" << std::endl;
}

#define ARG_CMP(n, str) (std::strcmp(argv[n], str) == 0)
#define ARG_IS_HELP(n) (ARG_CMP(n, "help") || ARG_CMP(n, "-h") || ARG_CMP(n, "--help"))
#define ARG_IS_VERSION(n) (ARG_CMP(n, "version") || ARG_CMP(n, "-v") || ARG_CMP(n, "--version"))

int main(int argc, const char* argv[])
{
#if DO_LOCAL_TEST == 0
    const fs::path main_dir = fs::path(getHomeDir()) / ".lct";
#else
    const fs::path main_dir = "test";
#endif

    const fs::path source_dir = main_dir / "archives";
    const fs::path install_dir = main_dir / "current";
    const fs::path bin_dir = install_dir / "bin";
    std::string bin_dir_string;
    for (char c : bin_dir.string()) {
        if (c == '\\') bin_dir_string += "\\\\";
        else bin_dir_string += c;
    }

    bool use_ansi = static_cast<bool>(supportsANSI());

    if (argc < 2) {
        printHelp(argv[0], std::cerr, use_ansi);
        return 1;
    }

    const fs::path state_file = main_dir / "lct.state";
    const std::string state_file_string = state_file.string();

    auto latestVersionIt = versions.find(latest_version);
    if (latestVersionIt == versions.end()) {
        std::cerr << "Internal Error: latest version not defined in versions" << std::endl;
        return 1;
    }

    State state;
    bool state_changed = false;
    if (!state.Load(state_file)) {
        printHelp(argv[0], std::cerr, use_ansi);
        return 1;
    }

    Command command = COMMAND_NONE;

    if      (ARG_IS_HELP(1))          command = COMMAND_HELP;
    else if (ARG_IS_VERSION(1))       command = COMMAND_VERSION;
    else if (ARG_CMP(1, "install"))   command = COMMAND_INSTALL;
    else if (ARG_CMP(1, "uninstall")) command = COMMAND_UNINSTALL;
    else if (ARG_CMP(1, "reinstall")) command = COMMAND_REINSTALL;
    else if (ARG_CMP(1, "update"))    command = COMMAND_UPDATE;
    else if (ARG_CMP(1, "list"))      command = COMMAND_LIST;
    else if (ARG_CMP(1, "path"))      command = COMMAND_PATH;
    else if (ARG_CMP(1, "remove"))    command = COMMAND_REMOVE;
    else if (ARG_CMP(1, "info"))      command = COMMAND_INFO;

    switch (command)
    {
        case COMMAND_HELP: {
            printHelp(argv[0], std::cout, use_ansi);
            break;
        }

        case COMMAND_VERSION: {
            printVersion(use_ansi);
            break;
        }

        case COMMAND_UNINSTALL: case COMMAND_REINSTALL: {
            std::vector<std::string> tools;
            std::unordered_set<std::string> added_tools;
            bool invalid_tool = false;

            for (int i = 2; i < argc; i++) {
                const char* arg = argv[i];

                if (arg[0] == '-') continue;

                std::string name = arg;
                
                auto bundleIt = bundles.find(name);
                if (bundleIt != bundles.end()) {
                    for (const std::string& tool : bundleIt->second) {
                        if (added_tools.insert(tool).second) tools.push_back(tool);
                    }
                } else {
                    if (added_tools.insert(name).second) tools.push_back(name);
                }
            }

            for (std::size_t i = 0; i < tools.size(); /* manual incrementing */) {
                const std::string& tool = tools[i];

                if (valid_tools_deps.find(tool) == valid_tools_deps.end()) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " doesn't exist. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;

                    continue;
                }

                if (!state.IsInstalled(tool)) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " isn't installed. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;

                    continue;
                }

                bool required_by_other = false;
                for (const auto& [other_tool, deps] : valid_tools_deps) {
                    if (other_tool == tool) continue;
                    if (!state.IsInstalled(other_tool)) continue;
                    if (added_tools.find(other_tool) != added_tools.end()) continue;

                    for (const auto& dep : deps) {
                        if (dep == tool) {
                            if (use_ansi) std::cerr << "\033[33m";
                            std::cerr << "Warning: Cannot uninstall " << tool  << ": still required by installed tool " << other_tool << ". Skipping.\n";
                            if (use_ansi) std::cerr << "\033[0m";
                            required_by_other = true;
                            break;
                        }
                    }
                    if (required_by_other) break;
                }

                if (required_by_other) {
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;
                    continue;
                }

                i++;
            }

            if (!tools.empty()) {
                try {
                    std::cout << "=> Uninstalling";

                    for (const std::string& tool : tools) {
                        std::cout << " ";
                        if (use_ansi) std::cout << "\033[32m";
                        std::cout << tool;
                        if (use_ansi) std::cout << "\033[0m";
                    }

                    std::cout << "..." << std::endl;

                    uninstall_version(install_dir, tools);
                    state_changed = true;

                    for (const std::string& tool : tools) {
                        state.RemoveTool(tool);
                    }
                } catch (const std::runtime_error& e) {
                    if (use_ansi) std::cerr << "\033[31m";
                    std::cerr << e.what() << std::endl;
                    if (use_ansi) std::cerr << "\033[0m"; 
                    return 1;
                }
            } else if (!invalid_tool) {
                printHelp(argv[0], std::cerr, use_ansi);
                return 1;
            }

            if (command != COMMAND_REINSTALL) break; // fall-through to install
        }

        case COMMAND_INSTALL: {
            std::vector<std::string> tools;
            std::unordered_set<std::string> added_tools;
            bool invalid_tool = false;

            for (int i = 2; i < argc; i++) {
                const char* arg = argv[i];

                if (arg[0] == '-') continue;

                std::string name = arg;
                
                auto bundleIt = bundles.find(name);
                if (bundleIt != bundles.end()) {
                    for (const std::string& tool : bundleIt->second) {
                        if (added_tools.insert(tool).second) tools.push_back(tool);
                    }
                } else {
                    if (added_tools.insert(name).second) tools.push_back(name);
                }
            }

            for (std::size_t i = 0; i < tools.size(); /* manual incrementing */) {
                const std::string& tool = tools[i];

                if (valid_tools_deps.find(tool) == valid_tools_deps.end()) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " doesn't exist. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;
                    continue;
                }

                if (state.IsInstalled(tool)) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " is already installed. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;
                    continue;
                }

                auto depsIt = valid_tools_deps.find(tool);
                if (depsIt != valid_tools_deps.end()) {
                    for (const std::string& dep : depsIt->second) {
                        if (state.IsInstalled(dep)) continue;

                        if (added_tools.insert(dep).second) {
                            if (use_ansi) std::cerr << "\033[33m";
                            std::cerr << "Warning: " << tool << " requires " << dep << ". Adding " << dep << "." << std::endl;
                            if (use_ansi) std::cerr << "\033[0m";
                            tools.push_back(dep);
                        }
                    }
                }

                i++;
            }

            if (!tools.empty()) {
                try {
                    std::cout << "=> Installing ";

                    for (const std::string& tool : tools) {
                        if (use_ansi) std::cout << "\033[32m";
                        std::cout << tool;
                        if (use_ansi) std::cout << "\033[0m";
                        std::cout << " ";
                    }

                    std::cout << "of ";
                    if (use_ansi) std::cout << "\033[36m";
                    std::cout << latest_version;
                    if (use_ansi) std::cout << "\033[0m";

                    std::cout << "..." << std::endl;

                    install_version(latest_version, source_dir, install_dir, tools, use_ansi);
                    state_changed = true;

                    for (const std::string& tool : tools) {
                        state.SetTool(tool, latest_version);
                    }
                } catch (const std::runtime_error& e) {
                    if (use_ansi) std::cerr << "\033[31m";
                    std::cerr << e.what() << std::endl;
                    if (use_ansi) std::cerr << "\033[0m"; 
                    return 1;
                }
            } else if (!invalid_tool) {
                printHelp(argv[0], std::cerr, use_ansi);
                return 1;
            }

            break;
        }

        case COMMAND_UPDATE: {
            std::vector<std::string> tools;
            std::unordered_set<std::string> added_tools;
            bool invalid_tool = false;

            for (int i = 2; i < argc; i++) {
                const char* arg = argv[i];

                if (arg[0] == '-') continue;

                std::string name = arg;
                
                auto bundleIt = bundles.find(name);
                if (bundleIt != bundles.end()) {
                    for (const std::string& tool : bundleIt->second) {
                        if (added_tools.insert(tool).second) tools.push_back(tool);
                    }
                } else {
                    if (added_tools.insert(name).second) tools.push_back(name);
                }
            }

            for (std::size_t i = 0; i < tools.size(); /* manual incrementing */) {
                const std::string& tool = tools[i];

                if (valid_tools_deps.find(tool) == valid_tools_deps.end()) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " doesn't exist. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;

                    continue;
                }

                if (!state.IsInstalled(tool)) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " isn't installed. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;

                    continue;
                }

                int version_value = -1;

                auto version = state.GetVersion(tool);
                if (version.has_value()) {
                    auto versionIt = versions.find(version->get());
                    if (versionIt != versions.end()) {
                        version_value = versionIt->second;
                    }
                }

                if (version_value >= latestVersionIt->second) {
                    if (use_ansi) std::cerr << "\033[33m";
                    std::cerr << "Warning: " << tool << " is already up to date. Skipping." << std::endl;
                    if (use_ansi) std::cerr << "\033[0m";
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;

                    continue;
                }

                bool required_by_other = false;
                for (const auto& [other_tool, deps] : valid_tools_deps) {
                    if (other_tool == tool) continue;
                    if (!state.IsInstalled(other_tool)) continue;
                    if (added_tools.find(other_tool) != added_tools.end()) continue;

                    for (const auto& dep : deps) {
                        if (dep == tool) {
                            if (use_ansi) std::cerr << "\033[33m";
                            std::cerr << "Warning: Cannot update " << tool  << ": still required by installed tool " << other_tool << ". Update both to update " << tool << ". Skipping.\n";
                            if (use_ansi) std::cerr << "\033[0m";
                            required_by_other = true;
                            break;
                        }
                    }
                    if (required_by_other) break;
                }

                if (required_by_other) {
                    tools.erase(tools.begin() + i);
                    invalid_tool = true;
                    continue;
                }

                i++;
            }

            if (!tools.empty()) {
                try {
                    std::cout << "=> Updating";

                    for (const std::string& tool : tools) {
                        std::cout << " ";
                        if (use_ansi) std::cout << "\033[32m";
                        std::cout << tool;
                        if (use_ansi) std::cout << "\033[0m";
                    }

                    std::cout << "..." << std::endl;

                    uninstall_version(install_dir, tools);
                    install_version(latest_version, source_dir, install_dir, tools, use_ansi);
                    state_changed = true;

                    for (const std::string& tool : tools) {
                        state.SetTool(tool, latest_version);
                    }
                } catch (const std::runtime_error& e) {
                    if (use_ansi) std::cerr << "\033[31m";
                    std::cerr << e.what() << std::endl;
                    if (use_ansi) std::cerr << "\033[0m"; 
                    return 1;
                }
            } else if (!invalid_tool) {
                printHelp(argv[0], std::cerr, use_ansi);
                return 1;
            }

            break;
        }

        case COMMAND_LIST: {
            for (const auto& kv : valid_tools_deps) {
                const std::string& tool = kv.first;
                const bool is_installed = state.IsInstalled(tool);

                if (use_ansi) std::cout << (is_installed ? "\033[32m" : "\033[31m");
                std::cout << tool << ": " << (is_installed ? "installed" : "not installed");
                if (use_ansi) std::cout << "\033[0m";

                if (is_installed) {
                    int version_value = -1;

                    auto version = state.GetVersion(tool);
                    if (version.has_value()) {
                        auto versionIt = versions.find(version->get());
                        if (versionIt != versions.end()) {
                            version_value = versionIt->second;
                        }

                        std::cout << " ";
                        if (use_ansi) {
                            if (version_value >= latestVersionIt->second) std::cout << "\033[36m";
                            else                                          std::cout << "\033[33m";
                        }
                        else {
                            if (version_value >= latestVersionIt->second) std::cout << "(";
                            else                                          std::cout << "!(";
                        }
                        std::cout << version->get();
                        if (use_ansi) std::cout << "\033[0m";
                        else          std::cout << ")";
                    }
                }

                if (!kv.second.empty()) {
                    std::cout << " | requires: ";
                    for (std::size_t i = 0; i < kv.second.size(); i++) {
                        std::cout << kv.second[i];
                        if (i + 1 < kv.second.size()) std::cout << ", ";
                    }
                }

                std::cout << std::endl;
            }

            break;
        }

        case COMMAND_PATH: {
            std::cout << "LCT-Directory: " << main_dir << std::endl;
            std::cout << std::endl;

            std::cout << "Add '" << bin_dir_string << "' to your PATH. If you don't know how to do this, follow these steps:" << std::endl;

#ifdef _WIN32
            std::cout << "1) Terminal:\n";
            std::cout << "   setx PATH \"%PATH%;" << bin_dir_string << "\"\n\n";
            std::cout << "   # or in PowerShell:\n";
            std::cout << "   [Environment]::SetEnvironmentVariable(\"PATH\", $env:PATH + \";" << bin_dir_string << "\", \"User\")\n\n";

            std::cout << "2) Current terminal:\n";
            std::cout << "   set PATH=%PATH%;" << bin_dir_string << "\n";
            std::cout << "   # or in PowerShell:\n";
            std::cout << "   $env:PATH += \";" << bin_dir_string << "\"\n\n";

            std::cout << "3) GUI apps (requires logout/login):\n";
            std::cout << "   # Changes via setx or PowerShell are picked up after logout/login\n";

#elif defined(__APPLE__) || defined(__MACH__)
            std::cout << "1) Terminal:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.zshrc\n";
            std::cout << "   # or when using bash:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.bashrc\n\n";

            std::cout << "2) Current terminal:\n";
            std::cout << "   export PATH=\"$PATH:" << bin_dir_string << "\"\n\n";

            std::cout << "3) GUI apps:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.zprofile\n";
            std::cout << "   # or when using bash:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.bash_profile\n";
            std::cout << "   # Effects take place after logout/login\n";

#elif defined(__linux__)
            std::cout << "1) Terminal:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.bashrc\n";
            std::cout << "   # or when using zsh:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.zshrc\n\n";

            std::cout << "2) Current terminal:\n";
            std::cout << "   export PATH=\"$PATH:" << bin_dir_string << "\"\n\n";

            std::cout << "3) GUI apps:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.profile\n";
            std::cout << "   # or when using zsh:\n";
            std::cout << "   echo 'export PATH=\"$PATH:" << bin_dir_string << "\"' >> ~/.zprofile\n";
            std::cout << "   # GUI apps will see it after logout/login\n";

#else
            std::cout << "Please use OS-specific commands to add \"" << bin_dir_string << "\" to PATH.\n";
#endif

            break;
        }

        case COMMAND_REMOVE: {
            sh_remove(main_dir.string().c_str());
            break;
        }
        
        case COMMAND_INFO: {
            printInfo(std::cout, argc, argv);
            break;
        }

        default:
            std::cerr << "Unknown command: " << argv[1] << std::endl;
            return 1;
    }

    if (state_changed) {
        sh_mkdir(state_file.parent_path().string().c_str());

        if (!state.Save(state_file)) {
            std::cerr << "Couldn't save state to " << state_file_string << std::endl;
            return 1;
        }
    }

    return 0;
}
