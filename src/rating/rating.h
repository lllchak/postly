#pragma once

#include "config.pb.h"
#include "enum.pb.h"

#include <string>
#include <unordered_map>

#include <nlohmann_json/json.hpp>

enum ERatingType {
    RT_LOG = 0,
    RT_RAW = 1,
    RT_ONE = 2
};

class TRating {
public:
    TRating() = default;
    explicit TRating(const std::string& configPath);

    void Load();
    double ScoreUrl(const std::string& url) const;

private:
    std::unordered_map<std::string, double> Records;
    postly::TAgencyConfig Config;
};

class TAlexaRating {
public:
    TAlexaRating() = default;
    explicit TAlexaRating(const std::string& configPath);

    void Load();
    double ScoreUrl(
        const std::string& host,
        const postly::ELanguage language,
        const ERatingType type,
        const double shift) const;
    double GetRawRating(const std::string& host) const;
    double GetCountryShare(const std::string& host, const std::string& code) const;

private:
    std::unordered_map<std::string, double> RawRating;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> CountryShare;
    postly::TAgencyConfig Config;
};
