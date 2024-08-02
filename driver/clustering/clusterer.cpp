#include "clusterer.h"
#include "impl/single_linkage.h"
#include "../utils.h"

#include <iostream>

namespace {

std::uint64_t GetIterTimestamp(const std::vector<TDBDocument>& docs, double percentile) {
    if (docs.empty()) {
        return 0;
    }
    assert(std::is_sorted(docs.begin(), docs.end(), [](const TDBDocument& d1, const TDBDocument& d2) {
        return d1.FetchTime < d2.FetchTime;
    }));

    std::size_t index = std::floor(percentile * docs.size());
    return docs[index].FetchTime;
}

}  // namespace

TClusterer::TClusterer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);
    for (const postly::TClusteringConfig& config : Config.clusterings()) {
        Clusterings[config.language()] = std::make_unique<TSlinkClustering>(config);
    }
}

TIndex TClusterer::Cluster(std::vector<TDBDocument>&& docs) const {
    std::stable_sort(docs.begin(), docs.end(),
        [](const TDBDocument& d1, const TDBDocument& d2) {
            if (d1.FetchTime == d2.FetchTime) {
                if (d1.Filename.empty() && d2.Filename.empty()) {
                    return d1.Title.length() < d2.Title.length();
                }
                return d1.Filename < d2.Filename;
            }
            return d1.FetchTime < d2.FetchTime;
        }
    );

    TIndex index;
    index.IterTimestamp = GetIterTimestamp(docs, Config.iter_timestamp_percentile());
    index.MaxTimestamp = docs.empty() ? 0 : docs.back().FetchTime;

    std::map<postly::ELanguage, std::vector<TDBDocument>> lang2Docs;
    while (!docs.empty()) {
        const TDBDocument& doc = docs.back();
        if (Clusterings.find(doc.Language) != Clusterings.end()) {
            lang2Docs[doc.Language].push_back(doc);
        }
        docs.pop_back();
    }
    docs.shrink_to_fit();
    docs.clear();

    for (const auto& [language, clustering] : Clusterings) {
        TClusters langClusters = clustering->Cluster(lang2Docs[language]);
        std::stable_sort(
            langClusters.begin(),
            langClusters.end(),
            [](const TCluster& a, const TCluster& b) {
                return a.GetMaxTimestamp() < b.GetMaxTimestamp();
            }
        );
        index.Clusters[language] = std::move(langClusters);
    }

    return index;
}
