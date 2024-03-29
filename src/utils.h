#pragma once

#include <string>
#include <iostream>

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

std::string GetHostFromUrl(const std::string& url);

std::string GetFilename(const std::string& path);

template <class TConfig>
void ParseConfig(const std::string& fname, TConfig& config) {
    const int fileDesc = open(fname.c_str(), O_RDONLY);
    ENSURE(fileDesc >= 0, "Could not open config file");
    google::protobuf::io::FileInputStream fileInput(fileDesc);
    const bool success = google::protobuf::TextFormat::Parse(&fileInput, &config);
    ENSURE(success, "Invalid protobug file");
}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
std::string ToString(T e) {
    return nlohmann::json(e);
}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
T FromString(const std::string& s) {
    return nlohmann::json(s).get<T>();
}

uint64_t DateToTimestamp(const std::string& date);
