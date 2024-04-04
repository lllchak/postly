#include "cluster.h"

#include "../rating/rating.h"
#include "../utils.h"

#include <cassert>
#include <cmath>
#include <set>
#include <vector>

#include <Eigen/Core>
#include <boost/range/algorithm/nth_element.hpp>

static constexpr std::size_t N_FEATURES = 120;

void TCluster::AddDocument(const TDBDocument& doc) {
    Documents.push_back(std::move(doc));
    MaxTimestamp = std::max(
        MaxTimestamp, static_cast<std::uint64_t>(Documents.back().FetchTime));
}

std::uint64_t TCluster::GetTimestamp(const float percentile) const {
    const std::size_t nDocs = GetSize();

    assert(nDocs);
    std::vector<std::uint64_t> clusterTs;
    clusterTs.reserve(nDocs);

    for (const auto& doc : Documents) {
        clusterTs.push_back(doc.FetchTime);
    }
    std::size_t idx = static_cast<std::size_t>(std::floor(percentile * (clusterTs.size() - 1)));
    boost::range::nth_element(clusterTs, clusterTs.begin() + idx);
    return clusterTs[idx];
}

void TCluster::Summarize(const TRating& agencyRating) {
    assert(GetSize());

    const auto embeddingKey =
        (GetLanguage() == postly::NL_RU ? postly::EK_FASTTEXT_TITLE : postly::EK_FASTTEXT_CLASSIC);
    const std::size_t embeddingSize = Documents.back().Embeddings.at(embeddingKey).size();

    Eigen::MatrixXf points(GetSize(), embeddingSize);
    for (std::size_t i = 0; i < GetSize(); i++) {
        auto embedding = Documents[i].Embeddings.at(embeddingKey);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
    }
    Eigen::MatrixXf docsCosine = points * points.transpose();

    std::vector<double> weights;
    weights.reserve(GetSize());
    std::uint64_t freshestTimestamp = GetMaxTimestamp();
    for (std::size_t i = 0; i < GetSize(); ++i) {
        const TDBDocument& doc = Documents[i];
        double docRelevance = docsCosine.row(i).mean();
        std::int64_t timeDiff =
            static_cast<std::int64_t>(doc.FetchTime) - static_cast<std::int64_t>(freshestTimestamp);
        double timeMultiplier = Sigmoid<double>(static_cast<double>(timeDiff) / 3600.0 + 12.0);
        double agencyScore = agencyRating.ScoreUrl(doc.Url);
        double weight = (agencyScore + docRelevance) * timeMultiplier;
        if (doc.IsNasty) {
            weight *= 0.5;
        }
        weights.push_back(weight);
    }
    SortByWeights(weights);
}

void TCluster::GetFeatures(const TAlexaRating& rating,
                           const std::vector<TDBDocument>& docs) {
    Features.reserve(N_FEATURES);
    const std::vector<std::string> codes = {"US", "GB", "IN", "RU", "CA", "AU"};
    const std::vector<double> decays = {1800., 3600., 7200., 86400.};
    const std::vector<double> shifts = {1., 1.3, 1.6};

    for (const double shift : shifts) {
        for (const double decay : decays) {
            auto features = GetImportance(rating, docs, postly::NL_EN, RT_LOG, shift, decay);
            Features.push_back(features.Importance);
            if (decay != 86400.) {
                continue;
            }
            for (const std::string& code : codes) {
                Features.push_back(features.WeightedCountryShare[code]);
            }
        }
    }

    for (ERatingType type : {RT_RAW, RT_ONE}) {
        for (double decay : decays) {
            auto features = GetImportance(rating, docs, postly::NL_EN, type, 0.0, decay);
            Features.push_back(features.Importance);
            if (decay != 86400.) {
                continue;
            }
            for (const std::string& code : codes) {
                Features.push_back(features.WeightedCountryShare[code]);
            }
        }
    }
}

TFeatures TCluster::GetImportance(const TAlexaRating& alexaRating,
                                  const std::vector<TDBDocument>& docs,
                                  const postly::ELanguage language,
                                  const ERatingType type,
                                  const double shift,
                                  const double decay) {
    TFeatures features;
    double count = 0.;
    double wCount = 0.;
    const std::vector<std::string> codes = {"US", "GB", "IN", "RU", "CA", "AU"};

    for (const std::string& code : codes) {
        features.CountryShare[code] = 0;
        features.WeightedCountryShare[code] = 0;
    }

    features.DocWeights.reserve(GetSize());
    for (const auto& doc : Documents) {
        const double agencyWeight =
            alexaRating.ScoreUrl(doc.Host, language, type, shift);
        features.DocWeights.push_back(agencyWeight);

        const std::string& docHost = doc.Host;;
        for (const std::string& code : codes) {
            double share = alexaRating.GetCountryShare(docHost, code);
            features.CountryShare[code] += share;
            features.WeightedCountryShare[code] += share * agencyWeight;
        }
        count += 1;
        wCount += agencyWeight;
    }

    for (const std::string& code : codes) {
        if (count > 0) {
            features.CountryShare[code] /= count;
        }
        if (wCount > 0) {
            features.WeightedCountryShare[code] /= wCount;
        }
    }

    for (std::size_t i = 0; i < docs.size(); ++i) {
        const TDBDocument& startDoc = docs[i];
        int32_t startTime = startDoc.FetchTime;
        double rank = 0.;
        std::set<std::string> seenHosts;

        for (std::size_t j = i; j < docs.size(); ++j) {
            const TDBDocument& doc = docs[j];
            const std::string& docHost = doc.Host;
            if (seenHosts.insert(docHost).second) {
                double agencyWeight = alexaRating.ScoreUrl(docHost, language, type, shift);
                double docTimestampRemapped =
                    static_cast<double>(startTime - static_cast<int32_t>(doc.FetchTime)) / decay;
                double timeMultiplier = Sigmoid<double>(std::max(docTimestampRemapped, -15.));
                double score = agencyWeight * timeMultiplier;
                rank += score;
            }
        }
        if (rank > features.Importance) {
            features.Importance = rank;
            features.BestTimestamp = startDoc.FetchTime;
        }
    }

    return features;
}

void TCluster::GetImportance(const TAlexaRating& alexaRating) {
    std::vector<TDBDocument> docs = GetDocuments();

    std::stable_sort(docs.begin(), docs.end(), [](const TDBDocument& p1, const TDBDocument& p2) {
        if (p1.FetchTime != p2.FetchTime) {
            return p1.FetchTime < p2.FetchTime;
        }
        return p1.Url < p2.Url;
    });

    GetFeatures(alexaRating, docs);
    TFeatures features = GetImportance(alexaRating, docs, postly::NL_EN, RT_LOG, 1., 3600);
    BestTimestamp = features.BestTimestamp;
    Importance = features.Importance;
    DocWeights = features.DocWeights;
    CountryShare = features.CountryShare;
    WeightedCountryShare = features.WeightedCountryShare;
}

void TCluster::GetCategory() {
    std::vector<std::size_t> categoryCount(postly::ECategory_ARRAYSIZE);

    for (const TDBDocument& doc : Documents) {
        postly::ECategory categ = doc.Category;
        assert(doc.IsNews());
        categoryCount[static_cast<size_t>(categ)] += 1;
    }

    auto it = std::max_element(categoryCount.begin(), categoryCount.end());
    Category = static_cast<postly::ECategory>(std::distance(categoryCount.begin(), it));
}

void TCluster::SortByWeights(const std::vector<double>& weights) {
    std::vector<std::pair<TDBDocument, double>> weightedDocs;
    weightedDocs.reserve(GetSize());

    for (size_t i = 0; i < GetSize(); i++) {
        weightedDocs.emplace_back(std::move(Documents[i]), weights[i]);
    }
    Documents.clear();

    std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](
        const std::pair<TDBDocument, double>& a,
        const std::pair<TDBDocument, double>& b)
    {
        if (std::abs(a.second - b.second) < 0.000001) {
            return a.first.Title < b.first.Title;
        }
        return a.second > b.second;
    });

    for (const auto& [doc, _] : weightedDocs) {
        AddDocument(doc);
    }
}

bool TCluster::operator<(const TCluster& rhs) const {
    if (MaxTimestamp == rhs.MaxTimestamp) {
        return Id < rhs.Id;
    }
    return MaxTimestamp < rhs.MaxTimestamp;
}

bool TCluster::operator<(const std::uint64_t timestamp) const {
    return MaxTimestamp < timestamp;
}
