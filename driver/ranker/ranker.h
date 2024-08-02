#pragma once

#include "driver/config.pb.h"

#include "../clustering/clustering.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

struct TWeight {
    std::uint64_t BestTime = 0;
    double Importance = 0.;
    double AgePenalty = 1.;
    double Weight = 0.;
    std::size_t ClusterSize = 0;
};

struct TWCluster {
    std::reference_wrapper<const TCluster> Cluster;
    TWeight Weight;

    TWCluster(const TCluster& cluster, const TWeight& weight)
        : Cluster(cluster)
        , Weight(weight)
    {
    }
};

class TRanker {
public:
    TRanker(const std::string& configPath);

    std::vector<std::vector<TWCluster>> Rank(
        TClusters::const_iterator begin,
        TClusters::const_iterator end,
        const std::uint64_t iterTimestamp,
        const std::uint64_t window) const;

private:
    postly::TRankerConfig Config;
};
