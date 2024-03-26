#include "ft_embedder.h"
#include "../utils.h"

#include <sstream>
#include <cassert>

#include <onmt/Tokenizer.h>

TFTEmbedder::TFTEmbedder(
    const std::string& embeddingModelPath,
    postly::EEmbedderField field,
    postly::EAggregationMode mode,
    std::size_t maxWords,
    const std::string& modelPath
)
    : TEmbedder(field)
    , Mode(mode)
    , MaxWords(maxWords)
{
    assert(!embeddingModelPath.empty());
    EmbeddingModel.loadModel(embeddingModelPath);
    LOG_DEBUG("FastText model loaded [" + embeddingModelPath + ']');

    if (!modelPath.empty()) {
        Model = torch::jit::load(modelPath);
        LOG_DEBUG("Torch model loaded [" + modelPath + ']');
    }
}

TFTEmbedder::TFTEmbedder(postly::TEmbedderConfig config)
    : TFTEmbedder(config.vector_model_path(),
                  config.embedder_field(),
                  config.aggregation_mode(),
                  config.max_words(),
                  config.model_path()) {}

TEmbedding TFTEmbedder::CalcEmbedding(const std::string& input) const {
    std::istringstream ss(input);
    const std::size_t embeddingSize = EmbeddingModel.getDimension();

    return std::vector<float>(embeddingSize);
}
