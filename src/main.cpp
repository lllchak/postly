#include "annotator.h"
#include "config.pb.h"
#include "db_document.h"
#include "document.h"
#include "embedder/ft_embedder.h"
#include "utils.h"

#include <iostream>
#include <optional>
#include <vector>

#include <boost/program_options.hpp>

int main(int argc, char** argv) {
    const auto vm = ParseOptions(argc, argv);

    postly::TAnnotatorConfig annotator_config;
    ParseConfig("configs/annotator.pbtxt", annotator_config);

    std::vector<std::string> langs = {"ru", "en"};
    TAnnotator annotator = TAnnotator("configs/annotator.pbtxt", langs, "top");

    std::vector<std::string> files;
    FilesFromDir(vm["input"].as<std::string>(), files, vm["ndocs"].as<std::size_t>());

    std::vector<TDBDocument> dbDocs = annotator.ProcessAll(files, postly::IF_HTML);

    return 0;
}
