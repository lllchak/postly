#pragma once

#include <string>
#include <iostream>

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
