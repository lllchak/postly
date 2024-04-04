#include "detect.h"

#include "../document/document.h"
#include "../utils.h"

#include <algorithm>
#include <utility>
#include <optional>
#include <sstream>
#include <vector>

inline static int FT_PREFIX_LEN = 9;

namespace {

std::optional<std::pair<std::string, double>>
GetCategory(const fasttext::FastText& model,
            const std::string& text,
            const double threshold) {
    std::string input = text;
    std::replace(input.begin(), input.end(), '\n', ' ');
    std::istringstream input_stream(input);

    std::vector<std::pair<fasttext::real, std::string>> pred;
    model.predictLine(input_stream, pred, 1, threshold);

    if (pred.empty()) {
        return std::nullopt;
    }

    const double prob = pred[0].first;
    const std::string label = pred[0].second.substr(FT_PREFIX_LEN);

    return std::make_pair(label, prob);
}

bool IsDirtyDoc(const TDocument& doc) {
    std::size_t realSize = 0;
    std::size_t badSymb = 0;
    std::size_t i = 0;

    while(i < doc.Title.size()) {
        unsigned char sym = (unsigned char)(doc.Title[i]);
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
            unsigned char sym2 = (unsigned char)(doc.Title[i]);

            if (((sym == 208) && (sym2 >= 144)) ||
                ((sym == 209) && (sym2 <= 143))) {
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
    const std::string docText(doc.Title + ' ' + doc.Description + ' ' + doc.Text.substr(0, 100));
    const auto categ = GetCategory(model, docText, 0.4);

    if (!categ.has_value()) {
        return postly::NL_UNDEFINED;
    }

    if (IsDirtyDoc(doc)) {
        return postly::NL_OTHER;
    }

    const auto& [label, prob] = categ.value();
    postly::ELanguage lang = FromString<postly::ELanguage>(label);
    if ((lang == postly::NL_RU && prob >= 0.6) ||
        (lang != postly::NL_RU && lang != postly::NL_UNDEFINED)) {
        return lang;
    }

    return postly::NL_OTHER;
}

postly::ECategory DetectCategory(const fasttext::FastText& model,
                                 const std::string& title,
                                 const std::string& text)
{
    const std::string input(title + " " + text);
    auto categ = GetCategory(model, input, 0.0);

    return categ ? FromString<postly::ECategory>(categ->first) : postly::NC_UNDEFINED;
}
