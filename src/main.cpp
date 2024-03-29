#include "annotator.h"
#include "config.pb.h"
#include "db_document.h"
#include "document.h"
#include "embedder/ft_embedder.h"
#include "utils.h"

#include <iostream>
#include <optional>
#include <vector>

int main() {
    postly::TAnnotatorConfig annotator_config;
    ParseConfig("configs/annotator.pbtxt", annotator_config);

    std::vector<std::string> langs = {"ru", "en"};
    TAnnotator annotator = TAnnotator("configs/annotator.pbtxt", langs, "top");

    TDocument document = TDocument("data/data/20191101/00/993065305003958.html");
    std::optional<TDBDocument> dbDoc = annotator.ProcessDocument(document);

    if (dbDoc.has_value()) {
        std::cout << '\"' << dbDoc->Title << "\" and " << '\"' << dbDoc->Text << "\" embedding:\n[";
        for (const float val : dbDoc->Embeddings[postly::EK_FASTTEXT_CLASSIC]) {
            std::cout << val << ", ";
        }
        std::cout << "]\n";
    }

    return 0;
}
