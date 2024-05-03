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

    if (vm["mode"].as<std::string>() == "server") {
        RunServer("configs/server.pbtxt", vm["input"].as<std::string>());
    }

    std::vector<std::string> langs = {"ru", "en"};
    TAnnotator annotator = TAnnotator("configs/annotator.pbtxt", langs, "top");
    TClusterer clusterer = TClusterer("configs/clusterer.pbtxt");
    TSummarizer summarizer = TSummarizer("configs/summarizer.pbtxt");
    TRanker ranker = TRanker("configs/ranker.pbtxt");

    std::vector<std::string> files;
    FilesFromDir(vm["input"].as<std::string>(), files, vm["ndocs"].as<std::size_t>());
    const bool saveNotNews = vm["save_not_news"].as<bool>();
    const bool debugMode = vm["debug_mode"].as<bool>();

    std::vector<TDBDocument> dbDocs = annotator.ProcessAll(files, postly::IF_HTML);

    TIndex clusteringIndex = clusterer.Cluster(std::move(dbDocs));

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
