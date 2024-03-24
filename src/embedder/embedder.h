#pragma once

#include "config.pb.h"
#include "enum.pb.h"

#include <vector>

class TEmbedder {
public:
    using TEmbedding = std::vector<float>;

    explicit TEmbedder(postly::EEmbedderField field = postly::EF_ALL) : Field(field) {}

    virtual ~TEmbedder() = default;

    virtual TEmbedding CalcEmbedding(const std::string& input) const = 0;

    TEmbedding CalcEmbedding(const std::string& title, const std::string& text) const {
        std::string input;
        if (Field == postly::EF_ALL) {
            input = title + " " + text;
        } else if (Field == postly::EF_TITLE) {
            input = title;
        } else if (Field == postly::EF_TEXT) {
            input = text;
        }
        return CalcEmbedding(input);
    }

private:
    postly::EEmbedderField Field;
};
