#include "annotator.h"

#include "../detect/detect.h"
#include "../embedder/impl/ft_embedder.h"
#include "../nasty/nasty.h"
#include "../utils.h"

#include <boost/algorithm/string/join.hpp>
#include <tinyxml2/tinyxml2.h>

#include <optional>

namespace {

static std::unique_ptr<IEmbedder>
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
    LLOG("FastText language detector loaded", ELogLevel::LL_INFO);

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
        LLOG("FastText:lang=" << ToString(lang) << " category model loaded", ELogLevel::LL_INFO);
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

std::vector<TDBDocument>
TAnnotator::ProcessAll(const std::vector<std::string>& filesNames,
                       const postly::EInputFormat inputFormat) const {
    TThreadPool threadPool;
    std::vector<TDBDocument> dbDocs;
    TFutures futures;

    FillFutures(filesNames, dbDocs, futures, threadPool, inputFormat);

    for (auto& futureDoc : futures) {
        std::optional<TDBDocument> doc = futureDoc.get();
        if (ValidateDoc(doc)) {
            dbDocs.push_back(std::move(doc.value()));
        }
    }
    futures.clear();
    dbDocs.shrink_to_fit();
    return dbDocs;
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

std::optional<TDBDocument> TAnnotator::ProcessHTML(const std::string& path) const {
    std::optional<TDocument> html = ParseHTML(path);
    return html.has_value() ? ProcessDocument(*html) : std::nullopt;
}

std::optional<TDBDocument> TAnnotator::ProcessHTML(const tinyxml2::XMLDocument& html,
                                                 const std::string& filename) const {
    std::optional<TDocument> doc = ParseHTML(html, filename);
    return doc.has_value() ? ProcessDocument(*doc) : std::nullopt;
}

std::optional<TDocument> TAnnotator::ParseHTML(const std::string& path) const {
    TDocument doc;
    try {
        doc.FromHTML(path.c_str(), Config.parse_links(), Config.shrink_text(), Config.max_words());
    } catch (...) {
        LLOG("Bad HTML [" << path << ']', ELogLevel::LL_WARN);
        return std::nullopt;
    }
    return doc;
}

std::optional<TDocument> TAnnotator::ParseHTML(const tinyxml2::XMLDocument& html,
                                               const std::string& filename) const {
    TDocument doc;
    try {
        doc.FromHTML(html, filename, Config.parse_links(), Config.shrink_text(), Config.max_words());
    } catch (...) {
        LLOG("Bad HTML [" << filename << ']', ELogLevel::LL_WARN);
        return std::nullopt;
    }
    return doc;
}

std::string TAnnotator::Tokenize(const std::string &text) const
{
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);

    return boost::join(tokens, " ");
}

void TAnnotator::FillFutures(const std::vector<std::string>& filesNames,
                             std::vector<TDBDocument>& dbDocs,
                             TFutures& futures,
                             TThreadPool& threadPool,
                             const postly::EInputFormat inputFormat) const {
    switch (inputFormat) {
        case postly::IF_HTML: {
            dbDocs.reserve(filesNames.size());
            futures.reserve(filesNames.size());
            for (const auto& path : filesNames) {
                using TFunc = std::optional<TDBDocument>(TAnnotator::*)(const std::string&) const;
                futures.push_back(threadPool.enqueue<TFunc>(&TAnnotator::ProcessHTML, this, path));
            }
            break;
        }
        case postly::IF_JSON: {
            std::vector<TDocument> parsedDocs;
            for (const auto& path: filesNames) {
                std::ifstream fileStream(path);
                nlohmann::json json;
                fileStream >> json;
                for (const auto& obj : json) {
                    parsedDocs.emplace_back(obj);
                }
            }
            parsedDocs.shrink_to_fit();
            dbDocs.reserve(parsedDocs.size());
            futures.reserve(parsedDocs.size());
            for (const auto& parsedDoc: parsedDocs) {
                futures.push_back(threadPool.enqueue(&TAnnotator::ProcessDocument, this, parsedDoc));
            }
            break;
        }
        case postly::IF_JSONL: {
            std::vector<TDocument> parsedDocs;
            for (const auto& path: filesNames) {
                std::ifstream fileStream(path);
                std::string record;
                while (std::getline(fileStream, record)) {
                    parsedDocs.emplace_back(nlohmann::json::parse(record));
                }
            }
            parsedDocs.shrink_to_fit();
            dbDocs.reserve(parsedDocs.size());
            futures.reserve(parsedDocs.size());
            for (const auto& parsedDoc: parsedDocs) {
                futures.push_back(threadPool.enqueue(&TAnnotator::ProcessDocument, this, parsedDoc));
            }
            break;
        }
        default: {
            ENSURE(false, "Inappropriate input format, got " << ToString(inputFormat));
            break;
        }
    }
}

bool TAnnotator::ValidateDoc(const std::optional<TDBDocument>& doc) const {
    if (!doc.has_value()) {
        return false;
    }
    if (Languages.find(doc->Language) == Languages.end()) {
        return false;
    }
    if (!doc->IsFullyIndexed()) {
        return false;
    }
    if (!doc->IsNews() && !SaveNotNews) {
        return false;
    }
    return true;
}
