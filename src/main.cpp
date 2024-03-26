#include "embedder/ft_embedder.h"
#include "config.pb.h"
#include "utils.h"

#include <vector>
#include <iostream>

int main() {
    postly::TEmbedderConfig ft_embedder_config;
    ParseConfig("configs/embedder.pbtxt", ft_embedder_config);

    TFTEmbedder ft_embedder(ft_embedder_config);

    std::vector<float> res = ft_embedder.CalcEmbedding("");

    std::cout << res.size() << std::endl;

    return 0;
}
