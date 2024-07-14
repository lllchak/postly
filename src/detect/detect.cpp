#include "detect.h"

#include "../document/document.h"
#include "../utils.h"

#include <algorithm>
#include <utility>
#include <optional>
#include <sstream>
#include <vector>

namespace {

std::optional<std::pair<std::string, double>>
GetCategory(const fasttext::FastText& model,
            const std::string& originalText,
            const double threshold) {
    std::string text = originalText;
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::istringstream ifs(text);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, threshold);
    if (predictions.empty()) {
        return std::nullopt;
    }
    double probability = predictions[0].first;
    const size_t FT_PREFIX_LENGTH = 9;
    const std::string label = predictions[0].second.substr(FT_PREFIX_LENGTH);
    return std::make_pair(label, probability);
}

bool IsDirtyDoc(const TDocument& doc) {
    std::size_t realSize = 0;
    std::size_t badSymb = 0;
    std::size_t i = 0;

    while(i < doc.Title.size()) {
        unsigned char sym = (unsigned char) doc.Title[i];
        if (sym <= 127) {
            ++realSize;
            ++i;
        } else if (sym >= 240) {
            ++realSize;
            ++badSymb;
            i += 4;
        } else if (sym >= 220) {
            ++realSize;
            ++badSymb;
            i += 3;
        } else {
            i += 1;
            if (i >= doc.Title.size()) {
                break;
            }
            unsigned char sym2 = (unsigned char) doc.Title[i];

            if (((sym == 208) && (sym2 >= 144)) || ((sym == 209) && (sym2 <= 143))) {
                ++realSize;
                i += 1;
            } else {
                ++realSize;
                ++badSymb;
            }
        }
    }

    if (badSymb * 2 > realSize) {
        return true;
    }
    return false;
}

}  // namespace

postly::ELanguage DetectLanguage(const fasttext::FastText& model,
                                 const TDocument& doc) {
    std::string sample(doc.Title + " " + doc.Description + " " + doc.Text.substr(0, 100));
    auto pair = GetCategory(model, sample, 0.4);
    if (!pair) {
        return postly::NL_UNDEFINED;
    }
    const std::string& label = pair->first;
    double probability = pair->second;

    if (IsDirtyDoc(doc)) {
        return postly::NL_OTHER;
    }

    postly::ELanguage lang = FromString<postly::ELanguage>(label);
    if ((lang == postly::NL_RU && probability >= 0.6) || (lang != postly::NL_RU && lang != postly::NL_UNDEFINED)) {
        return lang;
    }
    return postly::NL_OTHER;
}

postly::ECategory DetectCategory(const fasttext::FastText& model,
                                 const std::string& title,
                                 const std::string& text) {
    const std::string input(title + " " + text);
    auto categ = GetCategory(model, input, 0.0);

    return categ ? FromString<postly::ECategory>(categ->first) : postly::NC_UNDEFINED;
}
