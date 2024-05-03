#include "index.h"

#include "../../utils.h"

TServerIndex::TServerIndex(std::unique_ptr<TClusterer> clusterer,
                           std::unique_ptr<TSummarizer> summarizer,
                           rocksdb::DB* db)
    : Clusterer(std::move(clusterer))
    , Summarizer(std::move(summarizer))
    , Db(db)
{
}

namespace {

std::pair<std::vector<TDBDocument>, std::uint64_t>
GetDocs(rocksdb::DB* db) {
    rocksdb::ManagedSnapshot snapshot(db);

    rocksdb::ReadOptions ropt(true, true);
    ropt.snapshot = snapshot.snapshot();

    std::vector<TDBDocument> docs;
    std::uint64_t timestamp = 0;

    std::unique_ptr<rocksdb::Iterator> iter(db->NewIterator(ropt));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        const rocksdb::Slice value = iter->value();
        if (value.empty()) {
            continue;
        }

        TDBDocument doc;
        const bool succes = TDBDocument::ParseFromArray(value.data(), value.size(), &doc);
        if (!succes) {
            LLOG("Bad document in db: " << iter->key().ToString(), ELogLevel::LL_DEBUG);
            continue;
        }

        timestamp = std::max(timestamp, doc.FetchTime);
        docs.push_back(std::move(doc));
    }

    return std::make_pair(std::move(docs), timestamp);
}

void RemoveStaleDocs(rocksdb::DB* db, std::vector<TDBDocument>& docs, std::uint64_t timestamp) {
    rocksdb::WriteOptions wopt;
    for (const auto& doc : docs) {
        if (doc.IsStale(timestamp)) {
            db->Delete(wopt, doc.Filename);
            LLOG("Removed: " << doc.Filename, ELogLevel::LL_DEBUG);
        }
    }
    docs.erase(
        std::remove_if(
            docs.begin(),
            docs.end(),
            [timestamp](const auto& doc){ return doc.IsStale(timestamp); }
        ), 
        docs.end()
    );
}

}  // namespace

TIndex TServerIndex::Build() const {
    auto [docs, timestamp] = GetDocs(Db);
    LLOG("Read " << docs.size() << " docs; timestamp: " << timestamp, ELogLevel::LL_DEBUG);
    RemoveStaleDocs(Db, docs, timestamp);

    TIndex index = Clusterer->Cluster(std::move(docs));

    for (auto& [lang, clusters] : index.Clusters) {
        Summarizer->Summarize(clusters);
        LLOG(
            "Clustering output: " << ToString(lang) << " " << clusters.size() << " clusters",
            ELogLevel::LL_DEBUG
        );
    }

    return index;
};