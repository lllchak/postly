#include "controller.h"

#include "document.pb.h"

#include "../cluster/cluster.h"
#include "../document/document.h"
#include "../utils.h"

#include <optional>
#include <tinyxml2/tinyxml2.h>

namespace {

void BuildSimpleResponse(std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         drogon::HttpStatusCode code = drogon::k400BadRequest) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    callback(resp);
}

std::optional<std::int64_t> GetTtlHeader(const std::string& value) try {
    if (value == "no-cache") {
        return -1;
    }
    static constexpr std::size_t lenPrefix = std::char_traits<char>::length("max-age=");
    return static_cast<std::int64_t>(std::stoi(value.substr(lenPrefix)));
} catch (const std::exception& e) {
    return std::nullopt;
}

std::optional<std::int64_t> GetPeriod(const std::string& value) try {
    return static_cast<std::int64_t>(std::stoi(value));
} catch (const std::exception& e) {
    return std::nullopt;
}

std::optional<postly::ELanguage> GetLang(const std::string& value) {
    const postly::ELanguage lang = FromString<postly::ELanguage>(value);
    return lang != postly::NL_UNDEFINED ? std::make_optional(lang) : std::nullopt;
}

std::optional<postly::ECategory> GetCategory(const std::string& value) {
    const postly::ECategory category = FromString<postly::ECategory>(value);
    return category != postly::NC_UNDEFINED ? std::make_optional(category) : std::nullopt;
}

Json::Value ToJson(const TCluster& cluster) {
    Json::Value articles(Json::arrayValue);
    for (const auto& document : cluster.GetDocuments()) {
        articles.append(document.Filename);
    }

    Json::Value json(Json::objectValue);
    json["title"] = cluster.GetTitle();
    json["category"] = ToString(cluster.GetCategory());
    json["articles"] = std::move(articles);

    return json;
}

}  // namespace

void TController::Init(const TAtomic<TIndex>* index,
                       rocksdb::DB* db,
                       std::unique_ptr<TAnnotator> annotator,
                       std::unique_ptr<TRanker> ranker) {
    Index = index;
    DB = db;
    Annotator = std::move(annotator);
    Ranker = std::move(ranker);
    Initialized.store(true, std::memory_order_release);
}

bool
TController::IsReady(std::function<void(const drogon::HttpResponsePtr&)> &&callback) const {
    if (!Initialized.load(std::memory_order_acquire)) {
        BuildSimpleResponse(std::move(callback), drogon::k503ServiceUnavailable);
        return false;
    }
    return true;
}

std::optional<TDBDocument>
TController::GetDBDocFromReq(const drogon::HttpRequestPtr& req,
                             const std::string& fname) const {
    tinyxml2::XMLDocument html;

    const tinyxml2::XMLError code = html.Parse(req->bodyData(), req->bodyLength());
    if (code != tinyxml2::XML_SUCCESS) {
        return std::nullopt;
    }
    return Annotator->ProcessHTML(html, fname);
}

bool TController::IndexDBDoc(const TDBDocument& doc,
                             const std::string& fname) const {
    ENSURE(doc.IsFullyIndexed(), "Trying to index a document without required fields");

    std::string serializedDoc;
    bool ok = doc.ToProtoString(&serializedDoc);
    if (!ok) {
        return false;
    }
    const rocksdb::Status status = DB->Put(rocksdb::WriteOptions(), fname, serializedDoc);
    if (!status.ok()) {
        return false;
    }
    return true;
}

drogon::HttpStatusCode TController::GetCode(const std::string& fname,
                                            const drogon::HttpStatusCode createdCode,
                                            const drogon::HttpStatusCode existedCode) const {
    std::string val;
    const auto mayExist = DB->KeyMayExist(rocksdb::ReadOptions(), fname, &val);
    return mayExist ? existedCode : createdCode;
}

void TController::Put(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& fname) const {
    if (!IsReady(std::move(callback))) {
        return;
    }

    const std::optional<std::int64_t> ttl = GetTtlHeader(req->getHeader("Cache-Control"));
    if (!ttl.has_value() || ttl.value() == -1) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    std::optional<TDBDocument> doc = GetDBDocFromReq(req, fname);
    if (!doc.has_value()) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }
    doc->TTL = ttl.value();

    if (!doc->IsFullyIndexed()) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    const drogon::HttpStatusCode code = GetCode(fname, drogon::k201Created, drogon::k204NoContent);
    bool ok = IndexDBDoc(doc.value(), fname);
    if (!ok) {
        BuildSimpleResponse(std::move(callback), drogon::k500InternalServerError);
        return;
    }

    BuildSimpleResponse(std::move(callback), code);
}

void TController::Delete(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& fname) const {
    if (!IsReady(std::move(callback))) {
        return;
    }

    std::string val;
    const bool mayExist = DB->KeyMayExist(rocksdb::ReadOptions(), fname, &val);
    if (mayExist) {
        const rocksdb::Status status = DB->Delete(rocksdb::WriteOptions(), fname);
        if (!status.ok()) {
            BuildSimpleResponse(std::move(callback), drogon::k500InternalServerError);
            return;
        }
    }

    BuildSimpleResponse(
        std::move(callback), mayExist ? drogon::k204NoContent : drogon::k404NotFound);
}

void TController::Threads(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    if (!IsReady(std::move(callback))) {
        return;
    }

    const std::optional<std::uint64_t> period = GetPeriod(req->getParameter("period"));
    const std::optional<postly::ELanguage> lang = GetLang(req->getParameter("lang_code"));
    const std::optional<postly::ECategory> category = GetCategory(req->getParameter("category"));

    if (!period || !lang || !category) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    const std::shared_ptr<TIndex> index = Index->Get();

    const auto& clusters = index->Clusters.at(lang.value());
    const std::uint64_t fromTimestamp =
        index->MaxTimestamp > period.value() ? index->MaxTimestamp - period.value() : 0;

    const auto indexIt =
        std::lower_bound(clusters.cbegin(), clusters.cend(), fromTimestamp);
    const auto weightedClusters =
        Ranker->Rank(indexIt, clusters.cend(), index->IterTimestamp, period.value());
    const auto& categoryClusters = weightedClusters.at(category.value());

    Json::Value threads(Json::arrayValue);
    int limit = 1000;
    for (const auto& weightedCluster : categoryClusters) {
        if (limit <= 0) {
            break;
        }
        const TCluster& cluster = weightedCluster.Cluster.get();
        threads.append(ToJson(cluster));
        --limit;
    }

    Json::Value json(Json::objectValue);
    json["threads"] = std::move(threads);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}

void TController::Get(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& fname) const {
    if (!IsReady(std::move(callback))) {
        return;
    }

    std::string serializedDoc;
    const rocksdb::Status s = DB->Get(rocksdb::ReadOptions(), fname, &serializedDoc);

    Json::Value ret;
    ret["fname"] = fname;
    ret["status"] = s.ok() ? "FOUND" : "NOT FOUND";

    if (s.ok()) {
        postly::TDocumentProto doc;
        const auto suc = doc.ParseFromString(serializedDoc);
        ret["parsed"] = suc;
        ret["title"] = doc.title();
        ret["lang"] = doc.language();
        ret["category"] = doc.category();
        ret["pubtime"] = Json::UInt64(doc.pub_time());
        ret["fetchtime"] = Json::UInt64(doc.fetch_time());
        ret["ttl"] = Json::UInt64(doc.ttl());
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void TController::Post(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& fname) const {
    if (!IsReady(std::move(callback))) {
        return;
    }

    const std::optional<int64_t> ttl = GetTtlHeader(req->getHeader("Cache-Control"));
    if (!ttl) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    std::optional<TDBDocument> dbDoc = GetDBDocFromReq(req, fname);
    if (!dbDoc) {
        BuildSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    drogon::HttpStatusCode code = GetCode(fname, drogon::k201Created, drogon::k200OK);
    bool isIndexed = dbDoc->IsFullyIndexed() && ttl.value() != -1;
    if (isIndexed) {
        dbDoc->TTL = ttl.value();
        bool success = IndexDBDoc(dbDoc.value(), fname);
        if (!success) {
            BuildSimpleResponse(std::move(callback), drogon::k500InternalServerError);
            return;
        }
    } else {
        code = drogon::k200OK;
    }

    Json::Value json(Json::objectValue);
    json["lang_code"] = dbDoc->HasSupportedLanguage() ? ToString(dbDoc->Language) : Json::Value::null ;
    json["is_news"] = dbDoc->Category != postly::NC_UNDEFINED ? dbDoc->IsNews() : Json::Value::null;
    json["is_indexed"] = isIndexed;
    Json::Value categories(Json::arrayValue);
    if (dbDoc->IsNews()) {
        categories.append(ToString(dbDoc->Category));
    }
    json["categories"] = categories;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(code);
    callback(resp);
}
