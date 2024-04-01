#pragma once

#include "config.pb.h"

#include "../cluster/cluster.h"
#include "clustering.h"
#include "../document/impl/db_document.h"

#include <unordered_map>
#include <vector>
#include <memory>

struct TIndex {
    std::unordered_map<postly::ELanguage, TClusters> Clusters;
    std::uint64_t IterTimestamp = 0;
    std::uint64_t MaxTimestamp = 0;
};

class TClusterer {
public:
    TClusterer(const std::string& configPath);

    TIndex Cluster(std::vector<TDBDocument>&& docs) const;

private:
    postly::TClustererConfig Config;
    std::unordered_map<postly::ELanguage, std::unique_ptr<IClustering>> Clusterings;
};
