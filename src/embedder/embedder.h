#pragma once

#include "config.pb.h"
#include "enum.pb.h"

#include <vector>

class IEmbedder {
public:
    explicit IEmbedder(postly::EEmbedderField field = postly::EF_ALL) : Field(field) {}

    virtual ~IEmbedder() = default;

    virtual std::vector<float> CalcEmbedding(const std::string& input) const = 0;

    std::vector<float> CalcEmbedding(const std::string& title, const std::string& text) const {
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
