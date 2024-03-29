#pragma once

#include "db_document.h"

#include <fasttext.h>

struct TDocument;

namespace fasttext {
class FastText;
}  // namespace fasttext

postly::ELanguage DetectLanguage(const fasttext::FastText& model,
                                 const TDocument& doc);
postly::ECategory DetectCategory(const fasttext::FastText& model,
                                 const std::string& title,
                                 const std::string& text);
