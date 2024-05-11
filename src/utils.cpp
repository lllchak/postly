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
        LLOG("Parsing host from " << url << " failed", ELogLevel::LL_WARN);
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
    using namespace boost::program_options;

    options_description od("options");
    od.add_options()
        ("mode", value<std::string>()->required(), "mode")
        ("input", value<std::string>()->required(), "input")
        ("server_config", po::value<std::string>()->default_value("../configs/server.pbtxt"), "server_config")
        ("annotator_config", po::value<std::string>()->default_value("../configs/annotator.pbtxt"), "annotator_config")
        ("clusterer_config", po::value<std::string>()->default_value("../configs/clusterer.pbtxt"), "clusterer_config")
        ("summarizer_config", po::value<std::string>()->default_value("../configs/summarizer.pbtxt"), "summarizer_config")
        ("ranker_config", po::value<std::string>()->default_value("../configs/ranker.pbtxt"), "ranker_config")
        ("languages", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{"ru", "en"}, "ru en"), "languages")
        ("ndocs", value<std::size_t>()->default_value(-1), "ndocs")
        ("window_size", value<std::uint64_t>()->default_value(3600*8), "window_size")
        ("save_not_news", bool_switch()->default_value(false), "save_not_news")
        ("debug_mode", bool_switch()->default_value(false), "debug_mode")
        ("print_top_debug_info", po::bool_switch()->default_value(false), "print_top_debug_info")
    ;

    positional_options_description p;
    p.add("mode", 1);
    p.add("input", 1);

    command_line_parser parser{argc, argv};
    parser.options(od).positional(p);
    parsed_options parsed_options = parser.run();
    variables_map vm;
    store(parsed_options, vm);
    notify(vm);

    return vm;
}
