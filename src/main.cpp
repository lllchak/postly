#include "config.pb.h"

#include "annotator/annotator.h"
#include "clustering/clusterer.h"
#include "document/impl/db_document.h"
#include "document/document.h"
#include "embedder/impl/ft_embedder.h"
#include "summarizer/summarizer.h"
#include "utils.h"

#include <iostream>
#include <optional>
#include <vector>

#include <boost/program_options.hpp>

int main(int argc, char** argv) {
    const auto vm = ParseOptions(argc, argv);

    std::vector<std::string> langs = {"ru", "en"};
    TAnnotator annotator = TAnnotator("configs/annotator.pbtxt", langs, "top");
    TClusterer clusterer = TClusterer("configs/clusterer.pbtxt");
    TSummarizer summarizer = TSummarizer("configs/summarizer.pbtxt");

    std::vector<std::string> files;
    FilesFromDir(vm["input"].as<std::string>(), files, vm["ndocs"].as<std::size_t>());

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

    for (std::size_t i = 0; i < allClusters.size(); ++i) {
        std::cout << "Cluster " << i << " lang=" << ToString(allClusters[i].GetLanguage()) << ":\n";
        for (const auto& doc : allClusters[i].GetDocuments()) {
            std::cout << doc.Title << std::endl;
        }
    }

    return 0;
}
