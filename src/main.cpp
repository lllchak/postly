#include "config.pb.h"
#include "enum.pb.h"

#include "annotator/annotator.h"
#include "clustering/clusterer.h"
#include "document/impl/db_document.h"
#include "document/document.h"
#include "embedder/impl/ft_embedder.h"
#include "ranker/ranker.h"
#include "server/server.h"
#include "summarizer/summarizer.h"
#include "utils.h"

#include <iostream>
#include <optional>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#include <nlohmann_json/json.hpp>

namespace {

void BuildDebugInfo(nlohmann::json& object, const TWCluster& cluster) {
    object["article_weights"] = nlohmann::json::array();
    object["features"] = nlohmann::json::array();
    object["weight"] = cluster.Weight.Weight;
    object["importance"] = cluster.Weight.Importance;
    object["best_time"] = cluster.Weight.BestTime;
    object["age_penalty"] = cluster.Weight.AgePenalty;
    object["average_us"] = cluster.Cluster.get().GetCountryShare().at("US");
    object["w_average_us"] = cluster.Cluster.get().GetWeightedCountryShare().at("US");
    object["average_gb"] = cluster.Cluster.get().GetCountryShare().at("GB");
    object["w_average_gb"] = cluster.Cluster.get().GetWeightedCountryShare().at("GB");
    object["average_in"] = cluster.Cluster.get().GetCountryShare().at("IN");
    object["w_average_in"] = cluster.Cluster.get().GetWeightedCountryShare().at("IN");

    for (const auto& weight : cluster.Cluster.get().GetDocWeights()) {
        object["article_weights"].push_back(weight);
    }
    for (const auto& feature : cluster.Cluster.get().GetFeatures()) {
        object["features"].push_back(feature);
    }
}

int RunServer(const std::string& configPath,
              const std::string& port) {
    TServer server(configPath);

    const auto parsePort = [](const std::string& s) -> std::optional<std::uint16_t> {
        try {
            return boost::lexical_cast<std::uint16_t>(s);
        } catch (std::exception& e) {
            return std::nullopt;
        }
    };

    const std::optional<std::uint16_t> p = parsePort(port);
    if (!p.has_value()) {
        return -1;
    }

    return server.Run(p.value());
}

}  // namespace

int main(int argc, char** argv) {
    const auto vm = ParseOptions(argc, argv);

    std::string mode = vm["mode"].as<std::string>();

    if (mode == "server") {
        RunServer(vm["server_config"].as<std::string>(), vm["input"].as<std::string>());
    }

    std::vector<std::string> langs = vm["languages"].as<std::vector<std::string>>();
    TAnnotator annotator = TAnnotator(vm["annotator_config"].as<std::string>(), langs, mode);
    TClusterer clusterer = TClusterer(vm["clusterer_config"].as<std::string>());
    TSummarizer summarizer = TSummarizer(vm["summarizer_config"].as<std::string>());
    TRanker ranker = TRanker(vm["ranker_config"].as<std::string>());

    std::string input = vm["input"].as<std::string>();
    postly::EInputFormat inputFormat = postly::IF_UNDEFINED;
    std::vector<std::string> files;
    if (boost::algorithm::ends_with(input, ".json")) {
        inputFormat = postly::IF_JSON;
        files.push_back(input);
        LLOG("JSON input", ELogLevel::LL_INFO);
    } else if (boost::algorithm::ends_with(input, ".jsonl")) {
        inputFormat = postly::IF_JSONL;
        files.push_back(input);
        LLOG("JSONL input", ELogLevel::LL_INFO);
    } else {
        inputFormat = postly::IF_HTML;
        int nDocs = vm["ndocs"].as<int>();
        FilesFromDir(input, files, nDocs);
        LLOG("Dir input, n docs: " << files.size(), ELogLevel::LL_INFO);
    }
    const bool saveNotNews = vm["save_not_news"].as<bool>();
    const bool debugMode = vm["debug_mode"].as<bool>();

    std::vector<TDBDocument> dbDocs = annotator.ProcessAll(files, inputFormat);

    if (mode == "languages") {
        nlohmann::json outputJson = nlohmann::json::array();
        std::map<std::string, std::vector<std::string>> langToFiles;

        for (const TDBDocument& doc : dbDocs) {
            langToFiles[nlohmann::json(doc.Language)].push_back(GetFilename(doc.Filename));
        }
        for (const auto& pair : langToFiles) {
            const std::string& language = pair.first;
            const std::vector<std::string>& files = pair.second;
            nlohmann::json object = {
                {"lang_code", language},
                {"articles", files}
            };
            outputJson.push_back(object);
        }
        std::cout << outputJson.dump(4) << std::endl;
        return 0;
    } else if (mode == "categories") {
        nlohmann::json outputJson = nlohmann::json::array();
        std::vector<std::vector<std::string>> catToFiles(postly::ECategory_ARRAYSIZE);

        for (const TDBDocument& doc : dbDocs) {
            postly::ECategory category = doc.Category;
            if (category == postly::NC_UNDEFINED || (category == postly::NC_NOT_NEWS && !saveNotNews)) {
                continue;
            }
            catToFiles[static_cast<size_t>(category)].push_back(GetFilename(doc.Filename));
        }
        for (size_t i = 0; i < postly::ECategory_ARRAYSIZE; i++) {
            postly::ECategory category = static_cast<postly::ECategory>(i);
            if (category == postly::NC_UNDEFINED || category == postly::NC_ANY) {
                continue;
            }
            if (!saveNotNews && category == postly::NC_NOT_NEWS) {
                continue;
            }
            const std::vector<std::string>& files = catToFiles[i];
            nlohmann::json object = {
                {"category", category},
                {"articles", files}
            };
            outputJson.push_back(object);
        }
        std::cout << outputJson.dump(4) << std::endl;
        return 0;
    }

    TIndex clusteringIndex = clusterer.Cluster(std::move(dbDocs));

    if (mode == "threads") {
        nlohmann::json outputJson = nlohmann::json::array();
        for (const auto& [language, langClusters]: clusteringIndex.Clusters) {
            for (const auto& cluster : langClusters) {
                nlohmann::json files = nlohmann::json::array();
                for (const TDBDocument& doc : cluster.GetDocuments()) {
                    files.push_back(GetFilename(doc.Filename));
                }
                nlohmann::json object = {
                    {"title", cluster.GetTitle()},
                    {"articles", files}
                };
                outputJson.push_back(object);
            }
        }
        std::cout << outputJson.dump(4) << std::endl;
        return 0;
    }

    TClusters allClusters;
    for (const auto& lang: {postly::NL_EN, postly::NL_RU}) {
        if (clusteringIndex.Clusters.find(lang) == clusteringIndex.Clusters.end()) {
            continue;
        }
        summarizer.Summarize(clusteringIndex.Clusters.at(lang));
        std::copy(
            clusteringIndex.Clusters.at(lang).cbegin(),
            clusteringIndex.Clusters.at(lang).cend(),
            std::back_inserter(allClusters)
        );
    }

    const std::uint64_t window = vm["window_size"].as<std::uint64_t>();
    const auto rankedTop =
        ranker.Rank(allClusters.begin(), allClusters.end(), clusteringIndex.IterTimestamp, window);

    nlohmann::json outputJson = nlohmann::json::array();
    for (auto it = rankedTop.begin(); it != rankedTop.end(); ++it) {
        const auto category = static_cast<postly::ECategory>(std::distance(rankedTop.begin(), it));
        if (category == postly::NC_UNDEFINED) {
            continue;
        }
        if (!saveNotNews && category == postly::NC_NOT_NEWS) {
            continue;
        }

        nlohmann::json rubricTop = {
            {"category", category},
            {"threads", nlohmann::json::array()}
        };
        for (const auto& cluster : *it) {
            nlohmann::json object = {
                {"title", cluster.Cluster.get().GetTitle()},
                {"category", cluster.Cluster.get().GetCategory()},
                {"articles", nlohmann::json::array()},
            };
            for (const auto& doc : cluster.Cluster.get().GetDocuments()) {
                object["articles"].push_back(GetFilename(doc.Filename));
            }
            if (debugMode) {
                BuildDebugInfo(object, cluster);
            }
            rubricTop["threads"].push_back(object);
        }
        outputJson.push_back(rubricTop);
    }

    std::cout << outputJson.dump(4) << std::endl;

    return 0;
}
