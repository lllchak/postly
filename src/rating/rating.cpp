#include "rating.h"

#include "../utils.h"

#include <fstream>
#include <cmath>

#include <boost/algorithm/string.hpp>

TRating::TRating(const std::string& configPath) {
    ::ParseConfig(configPath, Config);
    Load();
}

void TRating::Load() {
    std::string line;
    std::ifstream rating(Config.filename());
    if (!rating.is_open()) {
        LOG_DEBUG("Rating file is not available");
        return;
    }

    while (std::getline(rating, line)) {
        std::vector<std::string> lineSplitted;
        boost::split(lineSplitted, line, boost::is_any_of("\t"));
        Records[lineSplitted[1]] = std::stod(lineSplitted[0]);
    }

    if (Config.set_min_as_unk()) {
        Config.set_unk_rating(
            std::min_element(Records.begin(), Records.end(),
                [](const std::pair<std::string, double>& item1, const std::pair<std::string, double>& item2) {
                    return item1.second < item2.second;
                }
            )->second);
    }
}

double TRating::ScoreUrl(const std::string& url) const {
    const auto iter = Records.find(GetHostFromUrl(url));
    return (iter != Records.end()) ? iter->second : Config.unk_rating();
}

TAlexaRating::TAlexaRating(const std::string& configPath) {
    ::ParseConfig(configPath, Config);
    Load();
}

void TAlexaRating::Load() {
    std::ifstream fileStream(Config.filename());
    nlohmann::json json;
    fileStream >> json;
;
    for (const nlohmann::json& agency : json) {
        std::string host = agency.at("host").get<std::string>();
        double rating = agency.at("rating").get<double>();
        RawRating[host] = rating;

        for (auto& [key, value] : agency.at("country").items()) {
            CountryShare[host][key] = value;
        }
    }
}

double TAlexaRating::GetCountryShare(const std::string& host, const std::string& code) const {
    const auto iter = CountryShare.find(host);

    if (iter == CountryShare.end()) {
        return 0.;
    }
    const auto iter2 = iter->second.find(code);
    if (iter2 == iter->second.end()) {
        return 0.;
    }

    return iter2->second;
}

double TAlexaRating::GetRawRating(const std::string& host) const {
    const auto iter = RawRating.find(host);
    return (iter != RawRating.end()) ? iter->second : Config.unk_rating();
}

double TAlexaRating::ScoreUrl(const std::string& host,
                              const postly::ELanguage language,
                              const ERatingType type,
                              const double shift) const {
    if (type == RT_ONE) {
        return 1.;
    }

    double raw = GetRawRating(host);
    double coeff = 0;

    if (language == postly::NL_EN) {
        coeff = (100. - GetCountryShare(host, "US") - GetCountryShare(host, "GB")) / 100.;
    } else {
        coeff = GetCountryShare(host, "RU");
    }

    if (type == RT_LOG) {
        return std::max(log(raw * coeff + shift), 0.3);
    } else if (type == RT_RAW) {
        return raw * coeff;
    }

    return 1.;
}
