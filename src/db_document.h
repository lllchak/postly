#pragma once

#include "document.pb.h"

#include "nlohmann_json/json.hpp"

#include <string>
#include <vector>
#include <unordered_map>

class TDBDocument {
public:
    using TEmbedding = std::vector<float>;

    std::string FileName;
    std::string Url;
    std::string SiteName;
    std::string Host;

    std::uint64_t PublicationTime;
    std::uint64_t FetchTime;
    std::uint64_t TTL;

    std::string Title;
    std::string Text;
    std::string Description;

    postly::ELanguage Language = postly::ELanguage::NL_UNDEFINED;
    postly::ECategory Category = postly::ECategory::NC_UNDEFINED;
    
    std::unordered_map<postly::EEmbeddingKey, TEmbedding> Embeddings;
    std::vector<std::string> OutLinks;
    
    bool IsNasty = false;

public:
    static TDBDocument FromProto(const postly::TDocumentProto& proto);
    static bool FromProtoString(const std::string& value, TDBDocument* document);
    static bool ParseFromArray(const void* data, int size, TDBDocument* document);

    postly::TDocumentProto ToProto() const;
    nlohmann::json ToJson() const;
    bool ToProtoString(std::string* protoString) const;

    bool IsRussian() const { return Language == postly::NL_RU; }
    bool IsEnglish() const { return Language == postly::NL_EN; }
    bool IsNews() const { return Category != postly::NC_NOT_NEWS && Category != postly::NC_UNDEFINED; }
    bool IsFullyIndexed() const { return Language != postly::NL_UNDEFINED && Category != postly::NC_UNDEFINED && !Embeddings.empty(); }
    bool HasSupportedLanguage() const { return Language != postly::NL_UNDEFINED && Language != postly::NL_OTHER; }

    bool IsStale(uint64_t timestamp) const { return timestamp > FetchTime + TTL; }
};
