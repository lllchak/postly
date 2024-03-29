#include "annotator.h"
#include "detect.h"
#include "embedder/ft_embedder.h"
#include "nasty.h"
#include "utils.h"

#include <boost/algorithm/string/join.hpp>
#include <tinyxml2/tinyxml2.h>

#include <optional>

namespace {

static std::unique_ptr<TEmbedder>
EmbedderFromConfig(const postly::TEmbedderConfig& config) {
    if (config.type() == postly::ET_FASTTEXT) {
        return std::make_unique<TFTEmbedder>(config);
    } else {
        ENSURE(false, "Bad embedder type");
    }
}

}  // namespace

TAnnotator::TAnnotator(const std::string& configPath,
                       const std::vector<std::string>& langs,
                       const std::string& mode)
        : Tokenizer(onmt::Tokenizer::Mode::Conservative,
                    onmt::Tokenizer::Flags::CaseFeature)
        , Mode(mode) {
    ParseConfig(configPath, Config);
    SaveNotNews |= Config.save_not_news();
    SaveTexts |= (Config.save_texts() || (mode == "json"));
    ComputeNasty |= Config.compute_nasty();

    LangDetector.loadModel(Config.lang_detect());
    LOG_DEBUG("FastText language detector loaded");

    for (const std::string& language : langs) {
        postly::ELanguage lang = FromString<postly::ELanguage>(language);
        Languages.insert(lang);
    }

    for (const auto& config : Config.category_models()) {
        const postly::ELanguage lang = config.language();
        if (Languages.find(lang) == Languages.end()) {
            continue;
        }
        CategDetectors[lang].loadModel(config.path());
        LOG_DEBUG("FastText:lang=" << ToString(lang) << " category model loaded");
    }

    for (const auto& config : Config.embedders()) {
        const postly::ELanguage lang = config.language();
        if (Languages.find(lang) == Languages.end()) {
            continue;
        }
        postly::EEmbeddingKey key = config.embedding_key();
        Embedders[std::make_pair(lang, key)] = EmbedderFromConfig(config);
    }
}

std::optional<TDBDocument>
TAnnotator::ProcessDocument(const TDocument& doc) const {
    TDBDocument dbDoc;
    dbDoc.Language = DetectLanguage(LangDetector, doc);
    dbDoc.Url = doc.Url;
    dbDoc.Host = GetHostFromUrl(dbDoc.Url);
    dbDoc.SiteName = doc.SiteName;
    dbDoc.Title = doc.Title;
    dbDoc.FetchTime = doc.FetchTime;
    dbDoc.PublicationTime = doc.PublicationTime;
    dbDoc.Filename = doc.Filename;

    if (SaveTexts) {
        dbDoc.Text = doc.Text;
        dbDoc.Description = doc.Description;
        dbDoc.OutLinks = doc.OutLinks;
    }

    if (Mode == "languages") {
        return dbDoc;
    }

    if (doc.Text.length() < Config.min_text_length()) {
        return dbDoc;
    }

    const std::string title = Tokenize(doc.Title);
    const std::string text = Tokenize(doc.Text);

    auto detectorIt = CategDetectors.find(dbDoc.Language);
    if (detectorIt != CategDetectors.end()) {
        const auto& detector = detectorIt->second;
        dbDoc.Category = DetectCategory(detector, title, text);
    }

    for (const auto& [embedInfo, embedder] : Embedders) {
        const auto& [lang, embedKey] = embedInfo;
        if (lang != dbDoc.Language) {
            continue;
        }
        const TDBDocument::TEmbedding docEmbed =
            embedder->CalcEmbedding(title, text);
        dbDoc.Embeddings.emplace(embedKey, std::move(docEmbed));
    }

    if (ComputeNasty) {
        dbDoc.IsNasty = IsNasty(dbDoc);
    }

    return dbDoc;
}

std::string TAnnotator::Tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);

    return boost::join(tokens, " ");
}
