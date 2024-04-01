#pragma once

#include "config.pb.h"

#include "../clustering.h"

#include <Eigen/Core>

class TSlinkClustering : public IClustering {
public:
    explicit TSlinkClustering(const postly::TClusteringConfig& config)
        : Config(config)
    {
    }

    TClusters Cluster(
        const std::vector<TDBDocument>& docs) const override;

private:
    Eigen::MatrixXf CalcDistances(
        const std::vector<TDBDocument>::const_iterator begin,
        const std::vector<TDBDocument>::const_iterator end,
        const std::unordered_map<postly::EEmbeddingKey, float>& embKeysWeights) const;
    std::vector<size_t> ClusterBatch(
        const std::vector<TDBDocument>::const_iterator begin,
        const std::vector<TDBDocument>::const_iterator end,
        const std::unordered_map<postly::EEmbeddingKey, float>& embKeysWeights) const;
    void LinkBatch(
        Eigen::MatrixXf& distances,
        std::vector<std::size_t>& labels,
        std::vector<std::size_t>& clustersSizes,
        std::vector<std::unordered_set<std::string>>& clustersSitesNames,
        std::vector<std::size_t>& nn,
        std::vector<float>& nnDistances,
        const std::size_t nDocs) const;

private:
    postly::TClusteringConfig Config;
};
