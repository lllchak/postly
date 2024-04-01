#include "document.h"
#include "utils.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <tinyxml2/tinyxml2.h>

#include <sstream>
#include <fstream>

TDocument::TDocument(const char* filename) {
    if (boost::algorithm::ends_with(filename, ".html")) {
        FromHTML(filename);
    } else if (boost::algorithm::ends_with(filename, ".json")) {
        FromJson(filename);
    }
}

TDocument::TDocument(const nlohmann::json& json) {
    FromJson(json);
}

TDocument::TDocument(const tinyxml2::XMLDocument& html,
                     const std::string& filename) {
    FromHTML(html, filename);
}

nlohmann::json TDocument::ToJson() const {
    nlohmann::json json({{"url", Url},
                         {"site_name", SiteName},
                         {"timestamp", FetchTime},
                         {"title", Title},
                         {"description", Description},
                         {"file_name", GetFilename(Filename)},
                         {"text", Text}});

    if (!OutLinks.empty()) {
        json["out_links"] = OutLinks;
    }

    return json;
}

void TDocument::FromJson(const char* filename) {
    std::ifstream fileStream(filename);
    nlohmann::json json;
    fileStream >> json;
    FromJson(json);
}

void TDocument::FromJson(const nlohmann::json& json) {
    json.at("url").get_to(Url);
    json.at("site_name").get_to(SiteName);
    json.at("timestamp").get_to(FetchTime);
    json.at("title").get_to(Title);
    json.at("description").get_to(Description);
    json.at("text").get_to(Text);

    if (json.contains("file_name")) {
        json.at("file_name").get_to(Filename);
    }

    if (json.contains("out_links")) {
        json.at("out_links").get_to(OutLinks);
    }
}

std::string GetFullText(const tinyxml2::XMLElement* element) {
    if (const tinyxml2::XMLText* textNode = element->ToText()) {
        return textNode->Value();
    }
    std::string text;
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (const tinyxml2::XMLElement* elementNode = node->ToElement()) {
            text += GetFullText(elementNode);
        } else if (const tinyxml2::XMLText* textNode = node->ToText()) {
            text += textNode->Value();
        }
        node = node->NextSibling();
    }
    return text;
}

void ParseLinksFromText(const tinyxml2::XMLElement* element,
                        std::vector<std::string>& links) {
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (const tinyxml2::XMLElement* nodeElement = node->ToElement()) {
            if (std::strcmp(nodeElement->Value(), "a") == 0
                && nodeElement->Attribute("href")) {
                links.push_back(nodeElement->Attribute("href"));
            }
            ParseLinksFromText(nodeElement, links);
        }
        node = node->NextSibling();
    }
}

void TDocument::FromHTML(const char* filename,
                         bool parseLinks,
                         bool shrinkText,
                         std::size_t maxWords) {
    if (!boost::filesystem::exists(filename)) {
        throw std::runtime_error("HTML file not found");
    }
    tinyxml2::XMLDocument originalDoc;
    originalDoc.LoadFile(filename);

    FromHTML(originalDoc, filename, parseLinks, shrinkText, maxWords);
}

void TDocument::FromHTML(const tinyxml2::XMLDocument& originalDoc,
                         const std::string& filename,
                         bool parseLinks,
                         bool shrinkText,
                         std::size_t maxWords) {
    Filename = filename;

    const tinyxml2::XMLElement* htmlElement = originalDoc.FirstChildElement("html");
    if (!htmlElement) {
        throw std::runtime_error("Parser error: no html tag");
    }
    const tinyxml2::XMLElement* headElement = htmlElement->FirstChildElement("head");
    if (!headElement) {
        throw std::runtime_error("Parser error: no head");
    }
    const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
    if (!metaElement) {
        throw std::runtime_error("Parser error: no meta");
    }
    while (metaElement != 0) {
        const char* property = metaElement->Attribute("property");
        const char* content = metaElement->Attribute("content");
        if (content == nullptr || property == nullptr) {
            metaElement = metaElement->NextSiblingElement("meta");
            continue;
        }
        if (std::strcmp(property, "og:title") == 0) {
            Title = content;
        }
        if (std::strcmp(property, "og:url") == 0) {
            Url = content;
        }
        if (std::strcmp(property, "og:site_name") == 0) {
            SiteName = content;
        }
        if (std::strcmp(property, "og:description") == 0) {
            Description = content;
        }
        if (std::strcmp(property, "article:published_time") == 0) {
            FetchTime = DateToTimestamp(content);
        }
        metaElement = metaElement->NextSiblingElement("meta");
    }
    const tinyxml2::XMLElement* bodyElement = htmlElement->FirstChildElement("body");
    if (!bodyElement) {
        throw std::runtime_error("Parser error: no body");
    }
    const tinyxml2::XMLElement* articleElement = bodyElement->FirstChildElement("article");
    if (!articleElement) {
        throw std::runtime_error("Parser error: no article");
    }
    const tinyxml2::XMLElement* pElement = articleElement->FirstChildElement("p");
    {
        std::vector<std::string> links;
        size_t wordCount = 0;
        while (pElement && (!shrinkText || wordCount < maxWords)) {
            std::string pText = GetFullText(pElement);
            if (shrinkText) {
                std::istringstream iss(pText);
                std::string word;
                while (iss >> word) {
                    wordCount++;
                }
            }
            Text += pText + "\n";
            if (parseLinks) {
                ParseLinksFromText(pElement, links);
            }
            pElement = pElement->NextSiblingElement("p");
        }
        OutLinks = std::move(links);
    }
    const tinyxml2::XMLElement* addressElement = articleElement->FirstChildElement("address");
    if (!addressElement) {
        return;
    }
    const tinyxml2::XMLElement* timeElement = addressElement->FirstChildElement("time");
    if (timeElement && timeElement->Attribute("datetime")) {
        PublicationTime = DateToTimestamp(timeElement->Attribute("datetime"));
    }
    const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
    if (aElement && aElement->Attribute("rel") && std::string(aElement->Attribute("rel")) == "author") {
        Author = aElement->GetText();
    }
}