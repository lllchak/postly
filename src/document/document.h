#pragma once

#include <nlohmann_json/json.hpp>

#include "enum.pb.h"

#include <string>
#include <vector>
#include <cstdint>

namespace tinyxml2 {
class XMLDocument;
}  // namespace tinyxml2

struct TDocument {
public:
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::string Description;
    std::string Filename;
    std::string Text;
    std::string Author;

    uint64_t PublicationTime = 0;
    uint64_t FetchTime = 0;

    std::vector<std::string> OutLinks;
    std::optional<postly::ELanguage> Language;

public:
    TDocument() = default;
    explicit TDocument(const char* fileName);
    explicit TDocument(const nlohmann::json& json);
    TDocument(const tinyxml2::XMLDocument& html,
              const std::string& fileName);

    nlohmann::json ToJson() const;
    void FromJson(const char* fileName);
    void FromJson(const nlohmann::json& json);
    void FromHTML(const char* fileName,
                  bool parseLinks=false,
                  bool shrinkText=false,
                  size_t maxWords=200);
    void FromHTML(const tinyxml2::XMLDocument& html,
                  const std::string& fileName,
                  bool parseLinks=false,
                  bool shrinkText=false,
                  size_t maxWords=200);
};