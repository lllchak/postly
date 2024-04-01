#include "config.pb.h"

#include "annotator/annotator.h"
#include "clustering/clusterer.h"
#include "document/impl/db_document.h"
#include "document/document.h"
#include "embedder/impl/ft_embedder.h"
#include "utils.h"

#include <iostream>
#include <optional>
#include <vector>

#include <boost/program_options.hpp>

int main(int argc, char** argv) {
    const auto vm = ParseOptions(argc, argv);

    std::vector<std::string> langs = {"ru", "en"};
    TAnnotator annotator = TAnnotator("configs/annotator.pbtxt", langs, "top");

    std::vector<std::string> files;
    FilesFromDir(vm["input"].as<std::string>(), files, vm["ndocs"].as<std::size_t>());

    std::vector<TDBDocument> dbDocs = annotator.ProcessAll(files, postly::IF_HTML);

    TClusterer clusterer = TClusterer("configs/clusterer.pbtxt");
    TIndex clusteringIndex = clusterer.Cluster(std::move(dbDocs));

    for (const auto& [language, langClusters]: clusteringIndex.Clusters) {
        std::cout << "Clusters for " << nlohmann::json(language) << " lang:\n";
        for (std::size_t i = 0; i < langClusters.size(); ++i) {
            std::cout << "Cluster " << i << ":\n";
            for (const auto& doc : langClusters[i].GetDocuments()) {
                std::cout << doc.Title << std::endl;
            }
        }
    }

    return 0;
}
