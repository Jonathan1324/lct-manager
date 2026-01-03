#include <iostream>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include "version/version.hpp"
#include "home/home.hpp"
#include "data/state.h"

namespace fs = std::filesystem;

typedef unsigned char Command;
#define COMMAND_NONE        ((Command)0)
#define COMMAND_INSTALL     ((Command)1)
#define COMMAND_UNINSTALL   ((Command)2)
#define COMMAND_REINSTALL   ((Command)3)

#define DO_LOCAL_TEST 1

const char* latest_version = "v0.1.0-alpha.5-jan2026.1";

int main(int argc, const char* argv[])
{
#if DO_LOCAL_TEST == 0
    const fs::path main_dir = fs::path(getHomeDir()) / ".lct";
#else
    const fs::path main_dir = "test";
#endif

    const fs::path source_dir = main_dir / "archives";
    const fs::path install_dir = main_dir / "current";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> <args>" << std::endl;
        return 1;
    }

    const fs::path state_file = main_dir / "lct.state";
    const std::string state_file_string = state_file.string();

    State state;
    if (State_Load(state_file_string.c_str(), &state) != 0) {
        std::cerr << "Couldn't load state from " << state_file_string << std::endl;
        return 1;
    }

    if (!state.version) state.version = latest_version;

    Command command = COMMAND_NONE;
    const char* cmd_str = argv[1];

    if      (std::strcmp(cmd_str, "install") == 0)   command = COMMAND_INSTALL;
    else if (std::strcmp(cmd_str, "uninstall") == 0) command = COMMAND_UNINSTALL;
    else if (std::strcmp(cmd_str, "reinstall") == 0) command = COMMAND_REINSTALL;

    switch (command)
    {
        case COMMAND_UNINSTALL: case COMMAND_REINSTALL: {
            if (!state.installed) {
                std::cerr << "=> LCT isn't installed" << std::endl;
                break;
            }

            try {
                std::cout << "=> Uninstalling " << state.version << "..." << std::endl;
                uninstall_version(install_dir);
                state.installed = 0;
            } catch (const std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            }

            if (command != COMMAND_REINSTALL) break; // fall-through to install
        }

        case COMMAND_INSTALL: {
            if (state.installed) {
                std::cerr << "=> LCT is already installed" << std::endl;
                break;
            }

            try {
                std::cout << "=> Installing " << state.version << "..." << std::endl;
                install_version(latest_version, source_dir, install_dir);
                state.installed = 1;
                state.version = latest_version;
            } catch (const std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            }

            break;
        }
        
        default:
            std::cerr << "Unknown command: " << cmd_str << std::endl;
            return 1;
    }

    if (State_Save(state_file_string.c_str(), &state) != 0) {
        std::cerr << "Couldn't save state to " << state_file_string << std::endl;
        return 1;
    }

    return 0;
}
