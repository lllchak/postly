#pragma once

#include "../cluster/cluster.h"
#include "../document/impl/db_document.h"

class IClustering {
public:
    IClustering() = default;
    virtual ~IClustering() = default;
    virtual TClusters Cluster(
        const std::vector<TDBDocument>& docs) const = 0;
};
