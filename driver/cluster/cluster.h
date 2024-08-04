#pragma once

#include "../document/impl/db_document.h"
#include "../rating/rating.h"
#include "../utils.h"

struct TFeatures {
    std::uint64_t BestTimestamp = 0;
    double Importance = 0.0;
    std::vector<double> DocWeights;
    std::map<std::string, double> CountryShare;
    std::map<std::string, double> WeightedCountryShare;
};

class TCluster {
private:
    std::uint64_t Id = 0;
    std::uint64_t MaxTimestamp = 0;

    postly::ECategory Category = postly::NC_UNDEFINED;
    std::uint64_t BestTimestamp = 0;
    double Importance = 0.0;
    std::vector<double> Features;
    std::vector<double> DocWeights;
    std::map<std::string, double> CountryShare;
    std::map<std::string, double> WeightedCountryShare;

    std::vector<TDBDocument> Documents;

public:
    explicit TCluster(const std::uint64_t id)
        : Id(id)
        {
        }

    void AddDocument(const TDBDocument& document);
    void Summarize(const TRating& agencyRating);

    void GetFeatures(
        const TAlexaRating& alexaRating,
        const std::vector<TDBDocument>& docs);

    TFeatures GetImportance(
        const TAlexaRating& alexaRating,
        const std::vector<TDBDocument>& docs,
        const postly::ELanguage language,
        const ERatingType type,
        const double shift,
        const double decay);

    void GetImportance(const TAlexaRating& alexaRating);
    void GetCategory();

    uint64_t GetTimestamp(const float percentile = 0.9) const;

    bool operator<(const TCluster& other) const;
    bool operator<(const std::uint64_t timestamp) const;

    postly::ECategory GetCategory() const { return Category; }
    std::uint64_t GetMaxTimestamp() const { return MaxTimestamp; }
    std::size_t GetSize() const { return Documents.size(); }
    const std::vector<TDBDocument>& GetDocuments() const { return Documents; }
    std::string GetTitle() const { return Documents.front().Title; }
    postly::ELanguage GetLanguage() const { return Documents.front().Language; }
    double GetImportance() const { return Importance; }
    std::uint64_t GetBestTimestamp() const { return BestTimestamp; }
    const std::vector<double>& GetDocWeights() const { return DocWeights; }
    const std::vector<double>& GetFeatures() const { return Features; }
    const std::map<std::string, double>& GetCountryShare() const { return CountryShare; }
    const std::map<std::string, double>& GetWeightedCountryShare() const { return WeightedCountryShare; }

private:
    void SortByWeights(const std::vector<double>& weights);
};

using TClusters = std::vector<TCluster>;