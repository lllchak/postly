#include "summarizer.h"

#include "../utils.h"

TSummarizer::TSummarizer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);

    Rating = TRating(Config.pagerank_rating());
    LOG("Pagerank rating loaded", ELogLevel::LL_INFO);

    AlexaRating = TAlexaRating(Config.alexa_rating());
    LOG("Alexa rating loaded", ELogLevel::LL_INFO);
}

void TSummarizer::Summarize(TClusters& clusters) const {
    for (auto& cluster : clusters) {
        assert(cluster.GetSize());
        cluster.Summarize(Rating);
        cluster.GetImportance(AlexaRating);
        cluster.GetCategory();
    }
}
