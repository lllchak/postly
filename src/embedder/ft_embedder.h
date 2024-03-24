#pragma once

#include "embedder.h"

#include <eigen/Core>
#include <fasttext.h>
#include <torch/script.h>

struct TDocument;

namespace fasttext {
class FastText;
}  // namespace fasttext

class TFTEmbedder : public TEmbedder {
public:
    TFTEmbedder(
        const std::string& vectorModelPath,
        postly::EEmbedderField field,
        postly::EAggregationMode mode,
        std::size_t maxWords);

    explicit TFTEmbedder(postly::TEmbedderConfig config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    postly::EAggregationMode Mode;
    fasttext::FastText VectorModel;
    std::size_t MaxWords;
    mutable torch::jit::script::Module Model;
}
