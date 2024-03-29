#include "utils.h"

#include <ctime>
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

uint64_t DateToTimestamp(const std::string& date) {
    std::regex ex("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)([+-])(\\d\\d):(\\d\\d)");
    std::smatch what;
    if (!std::regex_match(date, what, ex) || what.size() < 10) {
        throw std::runtime_error("wrong date format");
    }

    std::tm t = {};
    t.tm_sec = std::stoi(what[6]);
    t.tm_min = std::stoi(what[5]);
    t.tm_hour = std::stoi(what[4]);
    t.tm_mday = std::stoi(what[3]);
    t.tm_mon = std::stoi(what[2]) - 1;
    t.tm_year = std::stoi(what[1]) - 1900;

    time_t timestamp = timegm(&t);
    uint64_t zone_ts = std::stoi(what[8]) * 60 * 60 + std::stoi(what[9]) * 60;
    if (what[7] == "+") {
        timestamp = timestamp - zone_ts;
    } else if (what[7] == "-") {
        timestamp = timestamp + zone_ts;
    }
    return timestamp > 0 ? timestamp : 0;
}
