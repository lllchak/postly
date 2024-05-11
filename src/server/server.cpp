#include "server.h"

#include "config.pb.h"

#include "../annotator/annotator.h"
#include "../atomic/atomic.h"
#include "../clustering/clusterer.h"
#include "../controller/controller.h"
#include "../clustering/server/index.h"
#include "../utils.h"

#include <rocksdb/db.h>

#include <iostream>
#include <sys/resource.h>

namespace {

using TCallback = std::function<void(const drogon::HttpResponsePtr&)>;

std::optional<std::uint32_t> GetOpenFileLimit() {
    rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        return static_cast<std::uint32_t>(limit.rlim_cur);
    }
    return std::nullopt;
}

void CheckIO(const postly::TServerConfig& config) {
    const std::optional<std::uint32_t> limit = GetOpenFileLimit();
    if (!limit) {
        return;
    }

    if (config.max_connection_num() >= limit.value()) {
        LLOG(
            "ulimit -n is smaller than the \"max_connection_num\" option; overflow is possible",
            ELogLevel::LL_WARN
        );
    }

    if (config.db_max_open_files() >= limit.value()) {
        LLOG(
            "ulimit -n is smaller than the \"db_max_open_files\" option; overflow is possible",
            ELogLevel::LL_WARN
        );
    }
}

std::unique_ptr<rocksdb::DB> CreateDB(const postly::TServerConfig& config) {
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = !config.db_fail_if_missing();
    options.max_open_files = config.db_max_open_files();

    rocksdb::DB* db;
    const rocksdb::Status s = rocksdb::DB::Open(options, config.db_path(), &db);
    ENSURE(s.ok(), "Failed to create database: " << s.getState());

    return std::unique_ptr<rocksdb::DB>(db);
}

void InitServer(const postly::TServerConfig& config, const std::uint16_t port) {
    drogon::app()
        .setLogLevel(trantor::Logger::kTrace)
        .addListener("0.0.0.0", port)
        .setThreadNum(config.threads())
        .setMaxConnectionNum(config.max_connection_num())
        .setMaxConnectionNumPerIP(config.max_connection_num_per_ip())
        .setIdleConnectionTimeout(config.idle_connection_timeout())
        .setKeepaliveRequestsNumber(config.keepalive_requests_number())
        .setPipeliningRequestsNumber(config.pipelining_requests_number());
}

}  // namespace

TServer::TServer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);
}

int TServer::Run(const std::uint16_t port) {
    CheckIO(Config);
    LLOG("Parsed server config", ELogLevel::LL_INFO);

    LLOG("Creating database", ELogLevel::LL_DEBUG);
    std::unique_ptr<rocksdb::DB> db = CreateDB(Config);

    LLOG("Creating annotator", ELogLevel::LL_DEBUG);
    std::vector<std::string> languages = {"ru", "en"};
    std::unique_ptr<TAnnotator> annotator = std::make_unique<TAnnotator>(Config.annotator_config_path(), languages);

    LLOG("Creating clusterer", ELogLevel::LL_DEBUG);
    std::unique_ptr<TClusterer> clusterer = std::make_unique<TClusterer>(Config.clusterer_config_path());

    LLOG("Creating summarizer", ELogLevel::LL_DEBUG);
    std::unique_ptr<TSummarizer> summarizer = std::make_unique<TSummarizer>(Config.summarizer_config_path());

    LLOG("Creating ranker", ELogLevel::LL_DEBUG);
    std::unique_ptr<TRanker> ranker = std::make_unique<TRanker>(Config.ranker_config_path());

    TServerIndex serverIndex(std::move(clusterer), std::move(summarizer), db.get());

    LLOG("Launching server", ELogLevel::LL_DEBUG);
    InitServer(Config, port);

    auto controllerPtr = std::make_shared<TController>();
    drogon::app().registerController(controllerPtr);

    TAtomic<TIndex> index;
    drogon::DrClassMap::getSingleInstance<TController>()->Init(&index, db.get(), std::move(annotator), std::move(ranker));

    for (const auto& el : drogon::app().getHandlersInfo()) {
        LLOG(std::get<0>(el) << ' ' << std::get<1>(el) << ' ' << std::get<2>(el), ELogLevel::LL_DEBUG);
    }

    drogon::app().run();

    return 0;
}
