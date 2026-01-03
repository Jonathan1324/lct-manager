#include "home.hpp"

std::string getHomeDir()
{
#ifdef _WIN32
    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return std::string(homeDrive) + std::string(homePath);
    }
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) return std::string(userProfile);
    return "";
#else
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : "";
#endif
}
