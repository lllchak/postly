#pragma once

#include "../clusterer.h"
#include "../../summarizer/summarizer.h"

#include <rocksdb/db.h>

class TServerIndex {
public:
    TServerIndex(
        std::unique_ptr<TClusterer> clusterer,
        std::unique_ptr<TSummarizer> summarizer,
        rocksdb::DB* db
    );

    TIndex Build() const;

private:
    const std::unique_ptr<TClusterer> Clusterer;
    const std::unique_ptr<TSummarizer> Summarizer;
    rocksdb::DB* Db;
};
