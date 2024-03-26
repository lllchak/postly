#pragma once

#include <string>
#include <iostream>

#include <fcntl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

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
