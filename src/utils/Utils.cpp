#include "Utils.h"
#include "../AppContext.h"
#include "Logger.h"

#include <cstdlib>
#include <regex>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace fs = boost::filesystem;


std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}


std::string getEnvVar(const std::string& name, const std::string& default_val)
{
    const char* value = std::getenv(name.c_str());
    return value ? value : default_val;
}


fs::path find_executable_on_path(const std::string& name) {
    std::string path_env = getEnvVar("PATH", "");
    if (path_env.empty()) {
        return "";
    }

    std::vector<std::string> directories;
    boost::split(directories, path_env, boost::is_any_of(":"));

    for (const auto& dir : directories) {
        fs::path full_path = fs::path(dir) / name;
        // Check if the file exists and is executable
        boost::system::error_code ec;
        if (fs::exists(full_path, ec) && !ec && fs::is_regular_file(full_path, ec) && !ec) {
             // This is a basic check; a more robust one might check execute permissions
             return full_path;
        }
    }

    return ""; // Not found
}


// Re-usable from our previous work
std::string cleanModName(const std::string& filename) {
    fs::path p(filename);
    std::string name = p.stem().string();
    std::regex version_pattern1(R"((.+?)\s+-\s*\d+)");
    std::regex version_pattern2(R"((.+?)-\d{4,})");
    std::smatch match;

    if (std::regex_search(name, match, version_pattern1) || std::regex_search(name, match, version_pattern2)) {
        if (match.size() > 1) name = match[1].str();
    }

    name = std::regex_replace(name, std::regex(R"(\s*\([^)]*\))"), "");
    name = std::regex_replace(name, std::regex(R"([\s-.]+$)"), "");
    
    return name;
}

// New utility to get just the version part
std::string extractVersionString(const std::string& filename, const std::string& clean_name) {
    fs::path p(filename);
    std::string stem = p.stem().string();
    
    // Find the starting position of the version info by looking for where the clean name ends
    size_t pos = stem.find(clean_name);
    if (pos != std::string::npos) {
        std::string version_part = stem.substr(pos + clean_name.length());
        // Clean up leading characters like '-' or spaces
        version_part = std::regex_replace(version_part, std::regex("^(\\s*-\\s*|\\s+)"), "");
        if (!version_part.empty()) {
            return version_part;
        }
    }

    return "N/A";
}
