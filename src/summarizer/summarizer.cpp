#include "summarizer.h"
#include "../utils.h"

TSummarizer::TSummarizer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);

    Rating = TRating(Config.pagerank_rating());
    LOG_DEBUG("Pagerank rating loaded");

    AlexaRating = TAlexaRating(Config.alexa_rating());
    LOG_DEBUG("Alexa rating loaded");
}

void TSummarizer::Summarize(TClusters& clusters) const {
    for (auto& cluster : clusters) {
        assert(cluster.GetSize());
        cluster.Summarize(Rating);
        cluster.GetImportance(AlexaRating);
        cluster.GetCategory();
    }
}
