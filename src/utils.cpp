#include "utils.h"

#include <regex>

const std::regex
        hostRegex("(http|https)://(?:www\\.)?([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");

std::string GetHostFromUrl(const std::string& url) {
    std::string res_host = "";

    try {
        std::smatch match;
        if (std::regex_match(url, match, hostRegex) && match.size() >= 3) {
            res_host = std::string(match[2].first, match[2].second);
        }
    } catch (...) {
        LOG_DEBUG("Parsing host from " << url << " failed");
        return res_host;
    }

    return res_host;
}

std::string GetFilename(const std::string& path) {
    return path.substr(path.find_last_of("/") + 1);
}
