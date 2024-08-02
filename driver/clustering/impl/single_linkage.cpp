#include "single_linkage.h"

#include "../../utils.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

static const float INF = 1.0f;

void ApplyTimePenalty(const std::vector<TDBDocument>::const_iterator begin,
                      const std::size_t nDocs,
                      Eigen::MatrixXf& distances) {
    std::vector<TDBDocument>::const_iterator slow = begin;
    std::vector<TDBDocument>::const_iterator fast = begin + 1;

    for (std::size_t i = 0; i < nDocs; ++i, ++slow) {
        fast = slow + 1;
        for (std::size_t j = i + 1; j < nDocs; ++j, ++fast) {
            const std::uint64_t slowTs = slow->FetchTime;
            const std::uint64_t fastTs = fast->FetchTime;
            const std::uint64_t tsDiff = fastTs > slowTs ? fastTs - slowTs : slowTs - fastTs;
            const float hoursDiff = static_cast<float>(tsDiff) / 3600.0f;
            float penalty = 1.0f;

            if (hoursDiff >= 24.0f) {
                penalty = hoursDiff / 24.0f;
            }

            distances(i, j) = std::min(penalty * distances(i, j), INF);
            distances(j, i) = distances(i, j);
        }
    }
}

bool IsAppropriateSize(const std::size_t clusterSize,
                       const float dist,
                       const postly::TClusteringConfig& config) {
    if (clusterSize <= config.small_cluster_size()) {
        return true;
    }
    if (clusterSize <= config.medium_cluster_size()) {
        return dist <= config.medium_threshold();
    }
    if (clusterSize <= config.large_cluster_size()) {
        return dist <= config.large_threshold();
    }
    return false;
}

bool SetIntersection(const std::unordered_set<std::string> small,
                     const std::unordered_set<std::string> large) {
    return std::any_of(small.begin(), small.end(), [&large](const auto& siteName) {
        return large.find(siteName) != large.end();
    });
}

bool IsSourceIntersect(const std::unordered_set<std::string> lhs,
                       const std::unordered_set<std::string> rhs) {
    if (lhs.size() < rhs.size()) {
        return SetIntersection(lhs, rhs);
    }
    return SetIntersection(rhs, lhs);
}

}  // namspace

TClusters TSlinkClustering::Cluster(const std::vector<TDBDocument>& docs) const {
    std::unordered_map<postly::EEmbeddingKey, float> embKeysWeights;
    for (const auto& embKeyWeight : Config.embedding_keys_weights()) {
        embKeysWeights[embKeyWeight.embedding_key()] = embKeyWeight.weight();
    }

    const std::size_t nDocs = docs.size();
    const std::size_t intersectionSize = Config.intersection_size();

    std::vector<std::size_t> labels;
    labels.reserve(nDocs);

    std::vector<TDBDocument>::const_iterator begin = docs.cbegin();
    std::unordered_map<std::size_t, std::size_t> prev2currLabels;

    std::size_t batchStart = 0;
    std::size_t prevBatchEnd = batchStart;
    std::size_t maxLabel = 0;

    while (prevBatchEnd < nDocs) {
        const std::size_t nRemainingDocs = nDocs - batchStart;
        const std::size_t batchSize = std::min(
            nRemainingDocs, static_cast<std::size_t>(Config.chunk_size()));
        auto end = begin + batchSize;

        assert(begin->Url == docs[batchStart].Url);

        std::vector<std::size_t> currLabels = ClusterBatch(begin, end, embKeysWeights);
        std::for_each(currLabels.begin(), currLabels.end(), [&](std::size_t& i){ i += maxLabel; });
        maxLabel = *std::max_element(currLabels.begin(), currLabels.end());

        for (std::size_t i = batchSize;
             i < batchSize - intersectionSize && i < labels.size(); ++i) {
            std::size_t prevLabel = labels[i];
            std::size_t batchIdx = static_cast<std::size_t>(i - batchStart);
            std::size_t currLabel = currLabels.at(batchIdx);
            prev2currLabels[prevLabel] = currLabel;
        }
        if (!batchStart) {
            for (std::size_t i = 0; i < std::min(intersectionSize, currLabels.size()); ++i) {
                labels.push_back(currLabels[i]);
            }
        }
        for (std::size_t i = intersectionSize; i < currLabels.size(); ++i) {
            labels.push_back(currLabels[i]);
        }

        assert(batchStart == static_cast<std::size_t>(std::distance(docs.begin(), begin)));
        assert(batchStart + batchSize == static_cast<std::size_t>(std::distance(docs.begin(), end)));
        for (const auto& [lhs, rhs] : prev2currLabels) {
            UNUSED(lhs);
            UNUSED(rhs);
            assert(lhs < rhs);
        }

        prevBatchEnd = batchStart + batchSize;
        batchStart = prevBatchEnd - intersectionSize;
        begin = end - intersectionSize;
    }

    assert(labels.size() == nDocs);
    for (auto& label : labels) {
        auto it = prev2currLabels.find(label);
        if (it == prev2currLabels.end()) {
            continue;
        }
        label = it->second;
    }

    std::unordered_map<std::size_t, std::size_t> clustersLabels;
    TClusters clusters;
    for (std::size_t i = 0; i < nDocs; ++i) {
        const std::size_t clusterId = labels[i];
        auto it = clustersLabels.find(clusterId);
        if (it == clustersLabels.end()) {
            const std::size_t currLabel = clusters.size();
            clustersLabels[clusterId] = currLabel;
            clusters.emplace_back(currLabel);
            clusters[currLabel].AddDocument(docs[i]);
        } else {
            clusters[it->second].AddDocument(docs[i]);
        }
    }

    return clusters;
}

std::vector<std::size_t> TSlinkClustering::ClusterBatch(
        const std::vector<TDBDocument>::const_iterator begin,
        const std::vector<TDBDocument>::const_iterator end,
        const std::unordered_map<postly::EEmbeddingKey, float>& embKeysWeights) const {
    const std::size_t nDocs = std::distance(begin, end);
    assert(nDocs);

    Eigen::MatrixXf distances = CalcDistances(begin, end, embKeysWeights);

    if (Config.use_timestamp_moving()) {
        ApplyTimePenalty(begin, nDocs, distances);
    }

    std::vector<std::size_t> labels(nDocs);
    for (std::size_t i = 0; i < nDocs; ++i) { labels[i] = i; }

    std::vector<std::size_t> nn(nDocs);
    std::vector<float> nnDistances(nDocs);
    for (std::size_t i = 0; i < nDocs; ++i) {
        Eigen::Index minIdx;
        nnDistances[i] = distances.row(i).minCoeff(&minIdx);
        nn[i] = minIdx;
    }

    std::vector<std::size_t> clustersSizes(nDocs);
    std::vector<std::unordered_set<std::string>> clustersSitesNames(nDocs);
    auto it = begin;
    for (std::size_t i = 0; i < nDocs; ++i) {
        clustersSizes[i] = 1;
        if (Config.ban_same_hosts()) {
            clustersSitesNames[i].insert(it->SiteName);
            ++it;
        }
    }
    assert(!Config.ban_same_hosts() || it == end);

    LinkBatch(distances,
              labels,
              clustersSizes,
              clustersSitesNames,
              nn,
              nnDistances,
              nDocs);

    return labels;
}

Eigen::MatrixXf TSlinkClustering::CalcDistances(
        const std::vector<TDBDocument>::const_iterator begin,
        const std::vector<TDBDocument>::const_iterator end,
        const std::unordered_map<postly::EEmbeddingKey, float>& embKeysWeights) const {
    const std::size_t nDocs = std::distance(begin, end);
    assert(nDocs);
    Eigen::MatrixXf resDistances = Eigen::MatrixXf::Zero(nDocs, nDocs);

    for (const auto& [embKey, embWeight] : embKeysWeights) {
        const std::size_t embSize = begin->Embeddings.at(embKey).size();
        Eigen::MatrixXf points(nDocs, embSize);
        std::vector<std::size_t> badPoints;
        std::vector<TDBDocument>::const_iterator docs = begin;

        for (std::size_t i = 0; i < nDocs; ++i) {
            std::vector<float> emb = docs->Embeddings.at(embKey);
            Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> docVector(emb.data(), emb.size());

            if (std::abs(docVector.norm() - 0.0) > 1e-8) {
                points.row(i) = docVector / docVector.norm();
            } else {
                badPoints.push_back(i);
            }

            docs++;
        }

        Eigen::MatrixXf distances(nDocs, nDocs);
        distances = (-((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f) * embWeight;
        distances += distances.Identity(nDocs, nDocs) * embWeight;

        for (std::size_t idx : badPoints) {
            distances.row(idx) = Eigen::VectorXf::Ones(nDocs) * embWeight;
            distances.col(idx) = Eigen::VectorXf::Ones(nDocs) * embWeight;
        }

        distances = distances.cwiseMax(0.0f);
        resDistances += distances;
    }

    return resDistances;
}

void TSlinkClustering::LinkBatch(Eigen::MatrixXf& distances,
                                 std::vector<std::size_t>& labels,
                                 std::vector<std::size_t>& clustersSizes,
                                 std::vector<std::unordered_set<std::string>>& clustersSitesNames,
                                 std::vector<std::size_t>& nn,
                                 std::vector<float>& nnDistances,
                                 const std::size_t nDocs) const {
    float prevMinDist = 0.0f;

    for (std::size_t level = 0; level + 1 < nDocs; ++level) {
        auto minDistIt = std::min_element(nnDistances.begin(), nnDistances.end());
        const std::size_t minI = std::distance(nnDistances.begin(), minDistIt);
        const std::size_t minJ = nn[minI];
        const float minDist = *minDistIt;
        ENSURE(prevMinDist <= minDist, "SLINK non-decreasing distance invariant failed");
        prevMinDist = minDist;

        if (minDist > Config.small_threshold()) {
            break;
        }

        const std::size_t lhsClusterSize = clustersSizes[minI];
        const std::size_t rhsClusterSize = clustersSizes[minJ];
        const std::size_t newClusterSize = lhsClusterSize + rhsClusterSize;
        const bool isApporpriateSize = IsAppropriateSize(newClusterSize, minDist, Config);
        const bool isSourceIntersect = Config.ban_same_hosts() &&
                                       IsSourceIntersect(clustersSitesNames[minI], clustersSitesNames[minJ]);

        if (!isApporpriateSize || isSourceIntersect) {
            nnDistances[minI] = INF;
            nnDistances[minJ] = INF;
            distances(minI, minJ) = INF;
            distances(minJ, minI) = INF;

            for (std::size_t k = 0; k < static_cast<std::size_t>(distances.rows()); ++k) {
                if (k == minI || k == minJ) {
                    continue;
                }
                float iDistance = distances(minI, k);
                float jDistance = distances(minJ, k);
                if (iDistance < nnDistances[minI]) {
                    nnDistances[minI] = iDistance;
                    nn[minI] = k;
                }
                if (jDistance < nnDistances[minJ]) {
                    nnDistances[minJ] = jDistance;
                    nn[minJ] = k;
                }
            }
            continue;
        }

        for (std::size_t i = 0; i < nDocs; ++i) {
            if (labels[i] == minJ || labels[i] == labels[minJ]) {
                labels[i] = minI;
            }
        }

        clustersSizes[minI] = newClusterSize;
        clustersSizes[minJ] = newClusterSize;
        if (Config.ban_same_hosts()) {
            clustersSitesNames[minI].insert(clustersSitesNames[minJ].begin(), clustersSitesNames[minJ].end());
        }

        nnDistances[minI] = INF;
        for (std::size_t k = 0; k < static_cast<std::size_t>(distances.rows()); ++k) {
            if (k == minI || k == minJ) {
                continue;
            }
            const float newDistance = std::min(distances(minJ, k), distances(minI, k));
            distances(minI, k) = newDistance;
            distances(k, minI) = newDistance;
            if (newDistance < nnDistances[minI]) {
                nnDistances[minI] = newDistance;
                nn[minI] = k;
            }
            if (nn[k] == minJ || nn[k] == minI) {
                nnDistances[k] = newDistance;
                nn[k] = minI;
            }
        }

        nnDistances[minJ] = INF;
        for (std::size_t i = 0; i < static_cast<std::size_t>(distances.rows()); ++i) {
            distances(minJ, i) = INF;
            distances(i, minJ) = INF;
        }
    }
}
