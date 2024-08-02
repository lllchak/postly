#pragma once

#include "../embedder.h"

#include <Eigen/Core>
#include <fasttext.h>

struct TDocument;

namespace fasttext {
class FastText;
}  // namespace fasttext

class TFTEmbedder : public IEmbedder {
public:
    TFTEmbedder(
        const std::string& vectorModelPath,
        const postly::EEmbedderField field,
        const postly::EAggregationMode mode,
        const std::size_t maxWords);

    explicit TFTEmbedder(const postly::TEmbedderConfig& config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    postly::EAggregationMode Mode;
    fasttext::FastText EmbeddingModel;
    std::size_t MaxWords;
};
