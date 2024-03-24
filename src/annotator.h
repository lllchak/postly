#pragma once

#include "config.pb.h"

#include "db_document.h"
#include "embedder/embedder.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <onmt/Tokenizer.h>

struct TDocument;

namespace tinyxml2 {
class XMLDocument;
}  // namespace tinyxml2

class TAnnotator {
public:
    explicit TAnnotator(
        const std::string configPath,
        const std::vector<std::string>& langs,
        bool saveNotNews = false,
        const std::string& mode = "top");

    std::vector<TDBDocument> ProcessAll(
        const std::vector<std::string>& filesNames,
        postly::EInputFormat inputFormat) const;

    std::optional<TDBDocument> ProcessHTML(const std::string& path) const;
    std::optional<TDBDocument> ProcessHTML(
        const tinyxml2::XMLDocument& html, const std::string& filename) const;

private:
    std::optional<TDBDocument> ProcessDocument(const TDocument& document) const;

    std::optional<TDocument> ParseHtml(const std::string& path) const;
    std::optional<TDocument> ParseHtml(
        const tinyxml2::XMLDocument& html, const std::string& fileName) const;

    std::string PreprocessText(const std::string& text) const;

private:
    postly::TAnnotatorConfig Config;

    std::unordered_set<postly::ELanguage> Languages;
    onmt::Tokenizer Tokenizer;

    std::map<std::pair<postly::ELanguage, postly::EEmbeddingKey>, std::unique_ptr<TEmbedder>> Embedders;

    bool SaveNotNews = false;
    bool SaveTexts = false;
    bool ComputeNasty = false;
    std::string Mode;
};
