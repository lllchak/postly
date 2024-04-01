#pragma once

#include "enum.pb.h"

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <fcntl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <nlohmann_json/json.hpp>

#define ENSURE(CONDITION, MESSAGE)               \
    do {                                         \
        if (!(CONDITION)) {                      \
            std::ostringstream oss;              \
            oss << MESSAGE;                      \
            throw std::runtime_error(oss.str()); \
        }                                        \
    } while (false)

#define LOG_DEBUG(x) std::cerr << x << std::endl;

#define UNUSED(x) (void*)x

namespace postly {

NLOHMANN_JSON_SERIALIZE_ENUM(
    postly::ELanguage,
    {
        {postly::NL_UNDEFINED, nullptr},
        {postly::NL_RU, "ru"},
        {postly::NL_EN, "en"},
        {postly::NL_OTHER, "other"},
    }
)

NLOHMANN_JSON_SERIALIZE_ENUM(
    postly::ECategory,
    {
        {postly::NC_UNDEFINED, nullptr},
        {postly::NC_ANY, "any"},
        {postly::NC_SOCIETY, "society"},
        {postly::NC_ECONOMY, "economy"},
        {postly::NC_TECHNOLOGY, "technology"},
        {postly::NC_SPORTS, "sports"},
        {postly::NC_ENTERTAINMENT, "entertainment"},
        {postly::NC_SCIENCE, "science"},
        {postly::NC_OTHER, "other"},
        {postly::NC_NOT_NEWS, "not_news"},
    }
)

NLOHMANN_JSON_SERIALIZE_ENUM(
    postly::EInputFormat,
    {
        {postly::IF_UNDEFINED, nullptr},
        {postly::IF_HTML, "html"},
        {postly::IF_JSON, "json"},
        {postly::IF_JSONL, "json_long"},
    }
)

}  // namespace postly

std::string GetHostFromUrl(const std::string& url);

std::string GetFilename(const std::string& path);

template <class TConfig>
void ParseConfig(const std::string& fname, TConfig& config) {
    try {
        const int fileDesc = open(fname.c_str(), O_RDONLY);
        ENSURE(fileDesc >= 0, "Could not open config file");
        google::protobuf::io::FileInputStream fileInput(fileDesc);
        google::protobuf::TextFormat::Parse(&fileInput, &config);
    } catch (std::exception& e) {
        LOG_DEBUG("Error parsing config " << e.what());
    }
}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
std::string ToString(T e) {
    return nlohmann::json(e);
}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
T FromString(const std::string& s) {
    try {
        return nlohmann::json(s).get<T>();
    } catch (std::exception& e) {
        LOG_DEBUG(e.what());
        return T();
    }
}

uint64_t DateToTimestamp(const std::string& date);

void FilesFromDir(const std::string& dir,
                  std::vector<std::string>& dirFiles,
                  const std::size_t nDocs);

boost::program_options::variables_map
ParseOptions(const int argc, char** argv);
