#pragma once

#include "driver/config.pb.h"

#include "../rating/rating.h"
#include "../cluster/cluster.h"

#include <string>

class TSummarizer {
public:
    TSummarizer(const std::string& configPath);

    void Summarize(TClusters& clusters) const;

private:
    postly::TSummarizerConfig Config;
    TRating Rating;
    TAlexaRating AlexaRating;
};
