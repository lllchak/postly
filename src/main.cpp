#include "embedder/ft_embedder.h"
#include "config.pb.h"
#include "utils.h"
#include "document.h"

#include <vector>
#include <iostream>

int main() {
    postly::TEmbedderConfig ft_embedder_config;
    ParseConfig("configs/embedder.pbtxt", ft_embedder_config);

    TFTEmbedder ft_embedder(ft_embedder_config);

    std::vector<float> res = ft_embedder.CalcEmbedding("i hate programming");

    TDocument document = TDocument("data/data/20191101/00/993065305003958.html");

    return 0;
}
