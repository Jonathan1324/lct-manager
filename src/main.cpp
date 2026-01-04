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

namespace fs = std::filesystem;

typedef unsigned char Command;
#define COMMAND_NONE        ((Command)0)
#define COMMAND_INSTALL     ((Command)1)
#define COMMAND_UNINSTALL   ((Command)2)
#define COMMAND_REINSTALL   ((Command)3)
#define COMMAND_UPDATE      ((Command)4)
#define COMMAND_LIST        ((Command)5)

#define DO_LOCAL_TEST 1

const char* latest_version = "v0.1.0-alpha.5-jan2026.2";

static std::unordered_map<std::string, int> versions = {
    {"v0.1.0-alpha.5-jan2026.2", 0}
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

void printHelp(const char* name, std::ostream& out, bool use_ansi)
{
    out << "Usage: " << name << " <command> <args>" << std::endl;

    out << "> " << name << " install <tools>" << std::endl;
    out << "> " << name << " uninstall <tools>" << std::endl;
    out << "> " << name << " reinstall <tools>" << std::endl;
    out << "> " << name << " update <tools>" << std::endl;
    out << "> " << name << " list" << std::endl;
}

int main(int argc, const char* argv[])
{
#if DO_LOCAL_TEST == 0
    const fs::path main_dir = fs::path(getHomeDir()) / ".lct";
#else
    const fs::path main_dir = "test";
#endif

    const fs::path source_dir = main_dir / "archives";
    const fs::path install_dir = main_dir / "current";

    bool use_ansi = true;

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
    if (!state.Load(state_file)) {
        printHelp(argv[0], std::cerr, use_ansi);
        return 1;
    }

    Command command = COMMAND_NONE;
    const char* cmd_str = argv[1];

    if      (std::strcmp(cmd_str, "install") == 0)   command = COMMAND_INSTALL;
    else if (std::strcmp(cmd_str, "uninstall") == 0) command = COMMAND_UNINSTALL;
    else if (std::strcmp(cmd_str, "reinstall") == 0) command = COMMAND_REINSTALL;
    else if (std::strcmp(cmd_str, "update") == 0)    command = COMMAND_UPDATE;
    else if (std::strcmp(cmd_str, "list") == 0)      command = COMMAND_LIST;

    switch (command)
    {
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
                            if (use_ansi) std::cerr << "\033[0m";
                            std::cerr << "Cannot uninstall " << tool  << ": still required by installed tool " << other_tool << ".\n";
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
                            if (use_ansi) std::cerr << "\033[0m";
                            std::cerr << "Cannot update " << tool  << ": still required by installed tool " << other_tool << ". Update both to update " << tool << ".\n";
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
                    auto version = state.GetVersion(tool);
                    if (version.has_value()) {
                        std::cout << " ";
                        if (use_ansi) std::cout << "\033[36m";
                        else          std::cout << "(";
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
        
        default:
            std::cerr << "Unknown command: " << cmd_str << std::endl;
            return 1;
    }

    if (!state.Save(state_file)) {
        std::cerr << "Couldn't save state to " << state_file_string << std::endl;
        return 1;
    }

    return 0;
}
