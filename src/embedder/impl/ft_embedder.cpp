#include "ft_embedder.h"
#include "../../utils.h"

#include <sstream>
#include <cassert>

#include <onmt/Tokenizer.h>

TFTEmbedder::TFTEmbedder(
    const std::string& embeddingModelPath,
    const postly::EEmbedderField field,
    const postly::EAggregationMode mode,
    const std::size_t maxWords,
    const std::string& modelPath
)
    : IEmbedder(field)
    , Mode(mode)
    , MaxWords(maxWords)
{
    assert(!embeddingModelPath.empty());
    EmbeddingModel.loadModel(embeddingModelPath);
    LOG_DEBUG("FastText model loaded [" << embeddingModelPath << ']');

    if (!modelPath.empty()) {
        Model = torch::jit::load(modelPath);
        LOG_DEBUG("Torch model loaded [" << modelPath << ']');
    }
}

TFTEmbedder::TFTEmbedder(const postly::TEmbedderConfig& config)
    : TFTEmbedder(config.vector_model_path(),
                  config.embedder_field(),
                  config.aggregation_mode(),
                  config.max_words(),
                  config.model_path()) {}

std::vector<float> TFTEmbedder::CalcEmbedding(const std::string& input) const {
    std::istringstream input_stream(input);
    const std::size_t size = EmbeddingModel.getDimension();

    fasttext::Vector baseEmb(size);
    fasttext::Vector avgEmb(size);
    fasttext::Vector maxEmb(size);
    fasttext::Vector minEmb(size);

    std::string word;
    std::size_t n_words = 0;

    while (input_stream >> word) {
        if (n_words > MaxWords) {
            break;
        }

        EmbeddingModel.getWordVector(baseEmb, word);
        const float norm = baseEmb.norm();
        if (norm - 0.0 < 1e-6) {
            continue;
        }
        baseEmb.mul(1.0 / norm);

        if (n_words == 0) {
            maxEmb.addVector(baseEmb);
            minEmb.addVector(baseEmb);
        } else {
            for (std::size_t i = 0; i < size; ++i) {
                maxEmb[i] = std::max(maxEmb[i], baseEmb[i]);
                minEmb[i] = std::min(minEmb[i], baseEmb[i]);
            }
        }
        avgEmb.addVector(baseEmb);

        n_words++;
    }

    if (n_words > 0) {
        LOG_DEBUG("Processed " << n_words << " word(-s)");
        avgEmb.mul(1.0 / static_cast<float>(n_words));
    }

    if (Mode == postly::AM_AVG) {
        return std::vector<float>(avgEmb.data(), avgEmb.data() + avgEmb.size());
    } else if (Mode == postly::AM_MIN) {
        return std::vector<float>(minEmb.data(), minEmb.data() + minEmb.size());
    } else if (Mode == postly::AM_MAX) {
        return std::vector<float>(maxEmb.data(), maxEmb.data() + maxEmb.size());
    }

    assert(Mode == postly::AM_MATRIX);

    int dim = static_cast<int>(size);
    auto tensor = torch::zeros({dim * 3}, torch::requires_grad(false));

    tensor.slice(0, 0, dim) = torch::from_blob(avgEmb.data(), {dim});
    tensor.slice(0, dim, 2 * dim) = torch::from_blob(maxEmb.data(), {dim});
    tensor.slice(0, 2 * dim, 3 * dim) = torch::from_blob(minEmb.data(), {dim});

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor.unsqueeze(0));

    at::Tensor outputTensor = Model.forward(inputs).toTensor().squeeze(0).contiguous();
    float* outputTensorPtr = outputTensor.data_ptr<float>();
    size_t outputDim = outputTensor.size(0);

    std::vector<float> res(outputDim);
    for (size_t i = 0; i < outputDim; i++) {
        res[i] = outputTensorPtr[i];
    }

    return res;
}
