#pragma once

#include "../embedder.h"

#include <Eigen/Core>
#include <fasttext.h>
#include <torch/script.h>

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
        const std::size_t maxWords,
        const std::string& modelPath);

    explicit TFTEmbedder(const postly::TEmbedderConfig& config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    postly::EAggregationMode Mode;
    fasttext::FastText EmbeddingModel;
    std::size_t MaxWords;
    mutable torch::jit::script::Module Model;
};
