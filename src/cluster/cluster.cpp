#include "cluster.h"

void TCluster::AddDocument(const TDBDocument& doc) {
    Documents.push_back(std::move(doc));
    MaxTimestamp = std::max(
        MaxTimestamp, static_cast<std::uint64_t>(Documents.back().FetchTime));
}
