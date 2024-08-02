#pragma once

#include "driver/config.pb.h"

#include "../document/impl/db_document.h"
#include "../document/document.h"
#include "../embedder/embedder.h"
#include "../thread_pool/thread_pool.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <fasttext.h>
#include <onmt/Tokenizer.h>

struct TDocument;

namespace tinyxml2 {
class XMLDocument;
}  // namespace tinyxml2

namespace fasttext {
class FastText;
}  // namespace fasttext

using TFTCategDetectors =
    std::unordered_map<postly::ELanguage, fasttext::FastText>;

class TAnnotator {
public:
    explicit TAnnotator(
        const std::string& configPath,
        const std::vector<std::string>& langs,
        const std::string& mode = "top");

    std::vector<TDBDocument> ProcessAll(
        const std::vector<std::string>& filesNames,
        const postly::EInputFormat inputFormat) const;

    std::optional<TDBDocument> ProcessHTML(const std::string& path) const;
    std::optional<TDBDocument> ProcessHTML(
        const tinyxml2::XMLDocument& html, const std::string& filename) const;
    std::optional<TDBDocument> ProcessJson(const nlohmann::json& json) const;

private:
    std::optional<TDBDocument> ProcessDocument(const TDocument& document) const;

    std::optional<TDocument> ParseHTML(const std::string& path) const;
    std::optional<TDocument> ParseHTML(
        const tinyxml2::XMLDocument& html, const std::string& filename) const;
    std::optional<TDocument> ParseJson(const nlohmann::json& json) const;

    std::string Tokenize(const std::string& text) const;

private:
    postly::TAnnotatorConfig Config;

    std::unordered_set<postly::ELanguage> Languages;
    onmt::Tokenizer Tokenizer;

    fasttext::FastText LangDetector;
    TFTCategDetectors CategDetectors;
    std::map<std::pair<postly::ELanguage, postly::EEmbeddingKey>, std::unique_ptr<IEmbedder>> Embedders;

    std::string Mode;
    bool SaveNotNews = false;
    bool SaveTexts = false;
    bool ComputeNasty = false;
};
