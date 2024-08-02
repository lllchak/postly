#include "ranker.h"

#include "../utils.h"

namespace {

TWeight GetClusterWeight(const TCluster& cluster,
                         const std::uint64_t iterTimestamp,
                         const std::uint64_t window) {
    double timeMultiplier = 1.;
    const std::uint64_t clusterTime = cluster.GetBestTimestamp();

    if (clusterTime + window < iterTimestamp) {
        double clusterTimestampRemapped =
            static_cast<double>(clusterTime + window - iterTimestamp) / 3600.0 + 8.0;
        timeMultiplier = Sigmoid<double>(clusterTimestampRemapped);
    }

    double rank = cluster.GetImportance();

    return TWeight{clusterTime,
                   rank,
                   timeMultiplier,
                   rank * timeMultiplier,
                   cluster.GetSize()};
}

}  // namespace

TRanker::TRanker(const std::string& configPath) {
    ::ParseConfig(configPath, Config);
}

std::vector<std::vector<TWCluster>> TRanker::Rank(TClusters::const_iterator begin,
                                                  TClusters::const_iterator end,
                                                  const std::uint64_t iterTimestamp,
                                                  const std::uint64_t window) const {
    std::vector<TWCluster> weightedClusters;
    for (TClusters::const_iterator it = begin; it != end; it++) {
        const TCluster& cluster = *it;
        const TWeight weight = GetClusterWeight(cluster, iterTimestamp, window);
        weightedClusters.emplace_back(cluster, std::move(weight));
    }

    std::stable_sort(weightedClusters.begin(), weightedClusters.end(),
        [&](const TWCluster& a, const TWCluster& b) {
            const std::size_t leftSize = a.Weight.ClusterSize;
            const std::size_t rightSize = b.Weight.ClusterSize;
            if (leftSize == rightSize) {
                return a.Weight.Weight > b.Weight.Weight;
            }
            if (leftSize < this->Config.min_cluster_size() || rightSize < this->Config.min_cluster_size()) {
                return leftSize > rightSize;
            }
            return a.Weight.Weight > b.Weight.Weight;
        }
    );

    std::vector<std::vector<TWCluster>> output(postly::ECategory_ARRAYSIZE);
    for (const TWCluster& cluster : weightedClusters) {
        auto category = cluster.Cluster.get().GetCategory();
        assert(category != postly::NC_UNDEFINED && category != postly::NC_ANY);

        output[static_cast<std::size_t>(category)].push_back(cluster);
        output[static_cast<std::size_t>(postly::NC_ANY)].push_back(cluster);
    }

    return output;
}
