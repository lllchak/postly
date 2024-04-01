#include "utils.h"

#include <ctime>
#include <regex>

#include <boost/filesystem.hpp>

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

void FilesFromDir(const std::string& dir,
                  std::vector<std::string>& dirFiles,
                  const std::size_t nDocs) {
    boost::filesystem::path dirPath(dir);
    boost::filesystem::recursive_directory_iterator start(dirPath);
    boost::filesystem::recursive_directory_iterator end;

    for (auto it = start; it != end; ++it) {
        if (boost::filesystem::is_directory(it->path())) {
            continue;
        }
        std::string path = it->path().string();
        if (path.substr(path.length() - 5) == ".html") {
            dirFiles.push_back(path);
        }
        if (nDocs != -1 && dirFiles.size() == nDocs) {
            break;
        }
    }
}

boost::program_options::variables_map
ParseOptions(const int argc, char** argv) {
    boost::program_options::options_description od("options");
    od.add_options()
        ("input", boost::program_options::value<std::string>()->required(), "input")
        ("ndocs", boost::program_options::value<std::size_t>()->default_value(-1), "ndocs")
    ;

    boost::program_options::positional_options_description p;
    p.add("input", 1);

    boost::program_options::command_line_parser parser{argc, argv};
    parser.options(od).positional(p);
    boost::program_options::parsed_options parsed_options = parser.run();
    boost::program_options::variables_map vm;
    boost::program_options::store(parsed_options, vm);
    boost::program_options::notify(vm);
    return vm;
}
